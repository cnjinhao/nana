// http://nanapro.org/en-us/documentation/widgets/label.htm

#include <nana/gui.hpp>
#include <nana/gui/widgets/label.hpp>

int main()
{
    using namespace nana;
    form fm;
    label lb(fm, rectangle(fm.size()));
    lb.caption("Hello, <bold color=0xff0000 font=\"Consolas\" url=\"http://nanapro.org\">"
               "This is a bold red Consolas text</>");
    lb.format(true);
    lb.text_align(align::center, align_v::center);
    fm.show();
    exec();
    return 0;
}
