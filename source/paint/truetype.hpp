#ifndef TTF_HEADER_INCLUDED
#define TTF_HEADER_INCLUDED

#include <cstdint>
#include <fstream>
#include <nana/charset.hpp>
#include <nana/filesystem/filesystem.hpp>

namespace nana
{
	namespace spec
	{
		class truetype
		{
			struct tt_offset_table
			{
				std::uint16_t major_version;
				std::uint16_t minor_version;
				std::uint16_t num_of_tables;
				std::uint16_t search_range;
				std::uint16_t entry_selector;
				std::uint16_t range_shift;
			};

			struct tt_table_directory
			{
				char	name[4];		//table name
				std::uint32_t checksum;	//Check sum
				std::uint32_t offset;	//Offset from beginning of file
				std::uint32_t length;	//length of the table in bytes
			};

			struct tt_name_table_header
			{
				std::uint16_t format_selector;		//format selector. Always 0
				std::uint16_t name_records_count;	//Name Records count
				std::uint16_t storage_offset;		//Offset for strings storage, from start of the table
			};

			struct tt_name_record
			{
				std::uint16_t platform_id;
				std::uint16_t encoding_id;
				std::uint16_t language_id;
				std::uint16_t name_id;
				std::uint16_t string_length;
				std::uint16_t string_offset; //from start of storage area
			};
		public:
			using path_type = ::std::filesystem::path;

			truetype(const path_type& filename)
			{
				std::ifstream ifs(filename.string(), std::ios::binary);
				if (!ifs.is_open())
					return;

				tt_offset_table offset_table;
				if (ifs.read(reinterpret_cast<char*>(&offset_table), sizeof offset_table).gcount() != sizeof offset_table)
					return;

				const std::size_t num_of_tables = _m_swap(offset_table.num_of_tables);
				for (std::size_t i = 0; i < num_of_tables; ++i)
				{
					tt_table_directory table_directory;
					if (ifs.read(reinterpret_cast<char*>(&table_directory), sizeof table_directory).gcount() != sizeof table_directory)
						return;

					if (*reinterpret_cast<const std::uint32_t*>("name") == reinterpret_cast<std::uint32_t&>(table_directory.name))
					{
						//const std::size_t length = _m_swap(table_directory.length);
						const std::size_t directory_offset = _m_swap(table_directory.offset);

						ifs.seekg(directory_offset, std::ios::beg);

						tt_name_table_header name_table;
						if (ifs.read(reinterpret_cast<char*>(&name_table), sizeof name_table).gcount() != sizeof name_table)
							return;

						const std::size_t name_records_count = _m_swap(name_table.name_records_count);
						const std::size_t storage_offset = _m_swap(name_table.storage_offset);

						for (std::size_t u = 0; u < name_records_count; ++u)
						{
							tt_name_record record;
							if (ifs.read(reinterpret_cast<char*>(&record), sizeof record).gcount() != sizeof record)
								return;

							if ((0 == record.string_length) || (0x100 != record.name_id))
								continue;

							std::size_t string_length = _m_swap(record.string_length);

							auto const filepos = ifs.tellg();
							ifs.seekg(directory_offset + _m_swap(record.string_offset) + storage_offset, std::ios::beg);

							std::string text;

							//Check if it is unicode
							if ((0 == record.platform_id) || (record.platform_id == 0x300 && record.encoding_id == 0x100))
							{
								if (0 == (string_length & 1)) //the string_length must be
								{
									//This is unicode
									text.resize(string_length);
									ifs.read(&text.front(), string_length);

									for (std::size_t i = 0; i < string_length; i += 2)
										std::swap(text[i], text[i + 1]);
									
									text = ::nana::charset(text, nana::unicode::utf16).to_bytes(nana::unicode::utf8);
								}
							}
							else
							{
								text.resize(string_length);
								ifs.read(&text.front(), string_length);
							}

							if (!text.empty())
							{
								switch (record.name_id)
								{
								case 0x100:
									font_family_.swap(text);
									break;
								case 0x400:
									text.clear();
								}
							}
							ifs.seekg(filepos, std::ios::beg);
						}
					}


				}

			}

			const std::string& font_family() const
			{
				return font_family_;
			}
		private:
			static std::uint16_t _m_swap(std::uint16_t val)
			{
				return (val << 8) | (val >> 8);
			}

			static std::uint32_t _m_swap(std::uint32_t val)
			{
				return (static_cast<std::uint32_t>(_m_swap(std::uint16_t(val & 0xFFFF))) << 16) | _m_swap(std::uint16_t(val >> 16));
			}
		private:
			std::string font_family_;
		};
	}
}

#endif