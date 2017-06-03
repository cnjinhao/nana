// http://nanapro.org/en-us/blog/2016/05/an-introduction-to-nana-c-library/
// more: https://github.com/qPCR4vir/nana-demo

#include <nana/gui.hpp>
#include <nana/gui/widgets/label.hpp>

int main()
{
    using namespace nana;
    form fm;
    label lb(fm, rectangle(10, 10, 100, 30));
    lb.caption("Hello World!");
    fm.show();
    exec();
    return 0;
}
