#include <vector>
#include <memory>

#include "re2/re2.h"
#include "re2/re2_regex.h"

namespace duckdb_re2 {

bool RegexSearchInternal(const char *input, Match &match, const Regex &r, RE2::Anchor anchor, size_t start,
                         size_t end) {
	auto &regex = r.GetRegex();
	std::vector<StringPiece> target_groups;
	auto group_count = regex.NumberOfCapturingGroups();
	target_groups.resize(group_count + 1);
	match.groups.clear();
	if (!regex.Match(StringPiece(input), start, end, anchor, target_groups.data(), group_count)) {
		return false;
	}
	for (auto &group : target_groups) {
		GroupMatch group_match;
		group_match.text = group.ToString();
		group_match.position = group.data() - input;
		match.groups.emplace_back(group_match);
	}
	return true;
}

bool RegexSearch(const std::string &input, Match &match, const Regex &regex) {
	return RegexSearchInternal(input.c_str(), match, regex, RE2::UNANCHORED, 0, input.size());
}

bool RegexMatch(const std::string &input, Match &match, const Regex &regex) {
	return RegexSearchInternal(input.c_str(), match, regex, RE2::ANCHOR_BOTH, 0, input.size());
}

bool RegexMatch(const char *start, const char *end, Match &match, const Regex &regex) {
	return RegexSearchInternal(start, match, regex, RE2::ANCHOR_BOTH, 0, end - start);
}

bool RegexMatch(const std::string &input, const Regex &regex) {
	Match nop_match;
	return RegexSearchInternal(input.c_str(), nop_match, regex, RE2::ANCHOR_BOTH, 0, input.size());
}

std::vector<Match> RegexFindAll(const std::string &input, const Regex &regex) {
	std::vector<Match> matches;
	size_t position = 0;
	Match match;
	while (RegexSearchInternal(input.c_str(), match, regex, RE2::UNANCHORED, position, input.size())) {
		position += match.position(0) + match.length(0);
		matches.emplace_back(std::move(match));
	}
	return matches;
}

}