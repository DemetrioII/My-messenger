#include "include/application/commands/command_pattern_matcher.hpp"

void app::commands::CommandPatternMatcher::add_pattern(
    const std::string &command_name, const std::string &template_text) {
  patterns_.push_back(compile_pattern(command_name, template_text));
}

std::vector<std::string>
app::commands::CommandPatternMatcher::split_tokens(const std::string &text) {
  std::vector<std::string> string_tokens{};
  std::string cur = "";
  for (size_t i = 0; i < text.size(); ++i) {
    if (text[i] == ' ') {
      if (!cur.empty())
        string_tokens.push_back(cur);
      cur = "";
    } else {
      cur += text[i];
    }
  }
  string_tokens.push_back(cur);

  return string_tokens;
}

app::commands::CommandPattern
app::commands::CommandPatternMatcher::compile_pattern(
    const std::string &command_name, const std::string &template_text) {
  CommandPattern p;
  p.command_name = command_name;
  p.template_text = template_text;
  auto tokens = split_tokens(template_text);
  for (size_t i = 0; i < tokens.size(); ++i) {
    PatternToken token;
    if (tokens[i].ends_with(
            "...}")) { // for templates like /send {whom} {payload...}
      token.kind = PatternTokenKind::TailCapture;
      token.value = tokens[i].substr(1, tokens[i].size() - 4);
    } else if (tokens[i].starts_with('{')) {
      token.kind = PatternTokenKind::Capture;
      token.value = tokens[i].substr(1, tokens[i].size() - 2);
    } else {
      token.kind = PatternTokenKind::Literal;
      token.value = tokens[i];
    }
    p.tokens.push_back(token);
  }
  return p;
}

app::commands::CommandMatch app::commands::CommandPatternMatcher::try_match(
    const CommandPattern &pattern,
    const std::vector<std::string> &input_tokens) {
  CommandMatch out{.matched = false};
  size_t pi = 0, ii = 0;
  for (; pi < pattern.tokens.size(); ++pi) {
    auto token = pattern.tokens[pi];
    if (token.kind == PatternTokenKind::Literal) {
      if (ii >= input_tokens.size())
        return CommandMatch{.matched = false};
      if (input_tokens[ii] != token.value)
        return CommandMatch{.matched = false};
      ++ii;
      continue;
    }
    if (token.kind == PatternTokenKind::Capture) {
      if (ii >= input_tokens.size())
        return CommandMatch{.matched = false};
      out.captures.push_back(input_tokens[ii]);
      ++ii;
      continue;
    }
    if (token.kind == PatternTokenKind::TailCapture) {
      if (pi != pattern.tokens.size() - 1)
        return CommandMatch{.matched = false};
      std::string tail;
      for (size_t i = ii; i < input_tokens.size(); ++i) {
        if (!tail.empty()) {
          tail += ' ';
        }
        tail += input_tokens[i];
      }
      out.captures.push_back(tail);
      ii = input_tokens.size();
      break;
    }
  }
  if (ii == input_tokens.size()) {
    out.matched = true;
    out.command_name = pattern.command_name;
  }
  return out;
}

app::commands::CommandMatch
app::commands::CommandPatternMatcher::match(const std::string &input) const {
  auto tokens = split_tokens(input);
  for (size_t i = 0; i < patterns_.size(); ++i) {
    auto match = try_match(patterns_[i], tokens);
    if (match.matched)
      return match;
  }
  return CommandMatch{.matched = false};
}
