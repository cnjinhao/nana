// http://nanapro.org/en-us/documentation/widgets/button.htm

#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>

int main()
{
    using namespace nana;
    form fm;
    button btn(fm);
    btn.caption("Hello World!");
    btn.events().click(API::exit);

    // Define a layout object for the form.
    place layout(fm);
    layout.div("vert<><<><here weight=200><>><>");
    layout["here"] << btn;
    layout.collocate();

    fm.show();
    exec();
    return 0;
}
