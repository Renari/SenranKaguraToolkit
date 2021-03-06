#include "CAT.h"

#include "FileProcessing.h"

#include <algorithm>
#include <iostream>
#include <iomanip>

#include <stdexcept>

// #define RENAME_CONV0

namespace CAT {

	void load(std::istream& file, std::vector<ResourceEntry_t>& entries) {
		const std::streampos fileStart(file.tellg());

		// Read Header
		const uint32_t header_size(read<uint32_t>(file));
		ResourceEntry_t entry{};
		do {
			read(file, &entry.offset);
			if (entry.offset != 0xFFFFFFFF) {
				entries.push_back(entry);
			} else {
				break;
			}
		} while (file.tellg() < header_size + fileStart);

		// Read FileTable Data
		file.seekg(header_size + fileStart, std::ios::beg);

		const auto entryItter(entries.begin());
		const auto entriesEnd(entries.end());

		const auto max_level(3U);
		std::array<std::string, 3> level_build;
		auto curr_level = 0;
		auto itterOffset = -1;
		char c;
		do {
			c = file.get();
			switch (c) {
				case ',':
					{
						int next_level = curr_level + 1;
						if (next_level >= max_level) {
							report_and_throw("Next Level is unsupported!");
						}
						curr_level = next_level;
						break;
					}
				case 10:
					{
						if (!level_build[0].empty()) {
							// Absolute-Entry
							itterOffset++;
							if (itterOffset >= entries.size()) {
								std::cerr << "Entry out of scope" << std::endl;
								itterOffset--;
								if ((entryItter + itterOffset)->type != from_string(level_build[0])) {
									throw std::runtime_error(nullptr);
								}
							} else {
								(entryItter + itterOffset)->type = from_string(level_build[0]);
							}
							if (level_build[1].empty() || level_build[2].empty()) {
								throw std::runtime_error(nullptr);
							}
						} else {
							// Relative-Entry -> Subentry
							if (level_build[1].empty()) {
								level_build[1] = (entryItter + itterOffset)->sub_entries.back().package;
							}
						}
						ResourceEntry_t::SubEntry_t subEntry{ level_build[1], level_build[2] };
						(entryItter + itterOffset)->sub_entries.push_back(subEntry);

						// Reset State
						for (auto i(0); i < max_level; ++i) {
							level_build[i].clear();
						}
						curr_level = 0;
						break;
					}
				default:
					{
						level_build[curr_level] += c;
						break;
					}
			}
		} while (file.tellg() < entries[0].offset + fileStart);
	}
}

std::ostream& operator<<(std::ostream& out, const CAT::Header_t& header) {
	out << "Cat-Header:" << std::endl;
	out << "\tSize: " << std::dec << header.size << " bytes" << std::endl;
	for (const auto& offset : header.offsets) {
		out << "\tData-Offset: 0x" << std::hex << std::uppercase << offset << std::endl;
	}
	return out;
}

std::ostream& operator<<(std::ostream& out, const std::vector<CAT::ResourceEntry_t>& filetable) {
	out << "Cat-Entries:" << std::endl;
	for (const auto& res : filetable) {
		out << "\t" << to_string(res.type) << "-Entry (0x" << std::hex << res.offset << "): " << std::endl;
		for (const auto& subres : res.sub_entries) {
			out << "\t\tSub-Entry: " << subres.package << "/" << subres.resource << std::endl;
		}
	}
	return out;
}
