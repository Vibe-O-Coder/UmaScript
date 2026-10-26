#include "../src/bytecode.hpp"
#include "../src/native_runtime.hpp"
#include <cassert>
struct Node:uma_native::Object {
    inline static int live=0;
    std::shared_ptr<Node> next;
    uma_native::Fn<uma_native::I()> callback;
    Node(){++live;}~Node(){--live;}
    void clear()override{next.reset();callback={};}
    void trace(const std::function<void(const std::shared_ptr<uma_native::Object>&)>& visit)const override{uma_native::traceValue(next,visit);uma_native::traceValue(callback,visit);}
};
int main(){
    using namespace uma_native;
    Session session;
    auto root=std::make_shared<Node>();track(root);root->next=root;
    for(int i=0;i<100000;++i){auto node=std::make_shared<Node>();track(node);node->next=node;node->callback=makeClosure<I()>([](std::shared_ptr<Node> n){return n?I{7}:I{0};},node);auto arrayValue=uma_native::array<I>({1,2,3});}
    collect();assert(Node::live==1 && root->next==root);assert(heap.size()<10);
    root.reset();collect();assert(Node::live==0 && heap.empty());
    uma::Program program;uma::Runtime runtime(program);
    auto kept=std::make_shared<uma::Object>();kept->cls="C";runtime.objects.push_back(kept);
    for(int i=0;i<100000;++i){runtime.allocation();auto node=std::make_shared<uma::Object>();runtime.objects.push_back(node);auto function=uma::Value::function({});function.bound->captures.push_back(uma::Value::instance(node));node->fields["f"]={"fn(speed)",function};auto arrayValue=runtime.array("speed");}
    runtime.collect();assert(runtime.objects.size()==1 && runtime.arrays.empty());kept.reset();runtime.collect();assert(runtime.objects.empty());
    std::cout<<"100000 native cycles, 100000 VM closure cycles, arrays and live roots passed.\n";
}
