#ifndef NANA_UNICODE_BIDI_HPP
#define NANA_UNICODE_BIDI_HPP
#include <vector>

namespace nana
{
    class unicode_bidi
    {
    public:
        typedef wchar_t char_type;
        
        enum class directional_override_status
        {
            neutral, right_to_left, left_to_right   
        };

        enum class bidi_char
        {
            L, LRE, LRO, R, AL, RLE, RLO,
            PDF = 0x1000, EN, ES, ET, AN, CS, NSM, BN,
            B = 0x2000, S, WS, ON
        };

        enum class bidi_category
        {
            strong, weak = 0x1000, neutral = 0x2000
        };
        
        const static char_type LRE = 0x202A;
        const static char_type RLE = 0x202B;
        const static char_type PDF = 0x202C;
        const static char_type LRO = 0x202D;
        const static char_type RLO = 0x202E;
        const static char_type LRM = 0x200E;
        const static char_type RLM = 0x200F;
        
        struct remember
        {
            unsigned level;
            directional_override_status directional_override;   
        };
        
        struct entity
        {
            const wchar_t * begin, * end;
            bidi_char bidi_char_type;
            unsigned level;
        };

        void linestr(const char_type*, std::size_t len, std::vector<entity> & reordered);
    private:
        static unsigned _m_paragraph_level(const char_type * begin, const char_type * end);

        void _m_push_entity(const char_type * begin, const char_type *end, unsigned level, bidi_char);

        std::vector<entity>::iterator _m_search_first_character();

        bidi_char _m_eor(std::vector<entity>::iterator);

        void _m_resolve_weak_types();
        void _m_resolve_neutral_types();
        void _m_resolve_implicit_levels();
        void _m_reordering_resolved_levels(const char_type*, std::vector<entity> & reordered);
        static bidi_category _m_bidi_category(bidi_char);
        static bidi_char _m_char_dir(char_type);
    private:
        void _m_output_levels() const;
        void _m_output_bidi_char() const;
    private:
        std::vector<entity> levels_;
    };

}

#endif

 /* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */