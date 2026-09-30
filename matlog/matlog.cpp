#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <memory>
#include <vector>
#include <sstream>

template<typename F>
class OperationInfo {
    std::string name_;
    F evaluation_;

public:
    OperationInfo(std::string name, F evaluation)
        : name_(std::move(name)), evaluation_(std::move(evaluation)) {
    }

    const std::string &name() const noexcept { return name_; }
    const F &evaluation() const noexcept { return evaluation_; }
};


using BinaryInfo = OperationInfo<std::function<bool(bool, bool)> >;
using UnaryInfo = OperationInfo<std::function<bool(bool)> >;

const BinaryInfo BINARY_AND = {"&", [](const bool l, const bool r) { return l && r; }};
const BinaryInfo BINARY_OR = {"|", [](const bool l, const bool r) { return l || r; }};
const BinaryInfo IMPLIES = {"->", [](const bool l, const bool r) { return !l || r; }};
const UnaryInfo NOT_OP = {"!", [](const bool v) { return !v; }};

using estimation = std::unordered_map<std::string, bool>;

class LogicExpression {
protected:
    mutable std::string cached_;
    mutable bool cached_ok_ = false;

public:
    virtual const std::string &name() const = 0;

    virtual std::string to_syntax_tree() const = 0;

    virtual std::string to_string() const = 0;

    virtual bool evaluate(const estimation &) const = 0;

    virtual ~LogicExpression() = default;
};

using expression = std::unique_ptr<LogicExpression>;

class BinaryOperation : public LogicExpression {
    const BinaryInfo *info_;
    expression left_;
    expression right_;

public:
    BinaryOperation(const BinaryInfo *info,
                    expression left,
                    expression right)
        : info_(info), left_(std::move(left)), right_(std::move(right)) {
    }

    const std::string &name() const override { return info_->name(); }

    std::string to_syntax_tree() const override {
        return "(" + info_->name() + "," +
               left_->to_syntax_tree() + "," +
               right_->to_syntax_tree() + ")";
    }

    std::string to_string() const override {
        if (!cached_ok_) {
            cached_ = "(" + left_->to_string() + info_->name() + right_->to_string() + ")";
            cached_ok_ = true;
        }
        return cached_;
    }

    bool evaluate(const estimation &estim) const override {
        return info_->evaluation()(left_->evaluate(estim), right_->evaluate(estim));
    }

    const LogicExpression *left() const noexcept { return left_.get(); }
    LogicExpression *left() noexcept { return left_.get(); }

    const LogicExpression *right() const noexcept { return right_.get(); }
    LogicExpression *right() noexcept { return right_.get(); }
};

class UnaryOperation : public LogicExpression {
    const UnaryInfo *info_;
    expression operand_;

public:
    UnaryOperation(const UnaryInfo *info, expression operand)
        : info_(info), operand_(std::move(operand)) {
    }

    const std::string &name() const override { return info_->name(); }

    std::string to_syntax_tree() const override {
        return "(" + info_->name() + operand_->to_syntax_tree() + ")";
    }

    std::string to_string() const override {
        if (!cached_ok_) {
            cached_ = info_->name() + operand_->to_string();
            cached_ok_ = true;
        }
        return cached_;
    }

    bool evaluate(const estimation &estim) const override {
        return info_->evaluation()(operand_->evaluate(estim));
    }

    const LogicExpression *operand() const noexcept { return operand_.get(); }
    LogicExpression *operand() noexcept { return operand_.get(); }
};

class Variable final : public LogicExpression {
    std::string name_;

public:
    explicit Variable(std::string name) : name_(std::move(name)) {
    }

    const std::string &name() const override { return name_; }

    void set_name(std::string name) { name_ = std::move(name); }

    std::string to_syntax_tree() const override { return name_; }

    std::string to_string() const override {
        if (!cached_ok_) {
            cached_ = name_;
            cached_ok_ = true;
        }
        return cached_;
    }

    bool evaluate(const estimation &estim) const override {
        return estim.at(name_);
    }
};

const std::string LIE_NAME = "0";

class Lie final : public LogicExpression {
    const std::string &name() const override { return LIE_NAME; }
    std::string to_syntax_tree() const override { return LIE_NAME; }
    std::string to_string() const override { return LIE_NAME; }
    bool evaluate(const estimation &) const override { return false; }
};

class And final : public BinaryOperation {
public:
    And(expression l, expression r)
        : BinaryOperation(&BINARY_AND, std::move(l), std::move(r)) {
    }
};

class Or final : public BinaryOperation {
public:
    Or(expression l, expression r)
        : BinaryOperation(&BINARY_OR, std::move(l), std::move(r)) {
    }
};

class Implies final : public BinaryOperation {
public:
    Implies(expression l, expression r)
        : BinaryOperation(&IMPLIES, std::move(l), std::move(r)) {
    }
};

class Not final : public UnaryOperation {
public:
    explicit Not(expression op)
        : UnaryOperation(&NOT_OP, std::move(op)) {
    }
};

class InputCharSource {
    std::istream &in_;

public:
    explicit InputCharSource(std::istream &in = std::cin) : in_(in) {
    }

    bool has_next() { return in_.peek() != std::char_traits<char>::eof(); }
    char peek() { return static_cast<char>(in_.peek()); }
    char next() { return static_cast<char>(in_.get()); }

    template<class Pred>
    void skip_while(Pred pred) {
        while (has_next() && pred(static_cast<unsigned char>(peek()))) {
            next();
        }
    }

    void skip_ws() {
        skip_while([](const unsigned char c) { return std::isspace(c); });
    }
};

class ParseResult {
    expression expr_;
    std::unordered_set<std::string> vars_;

public:
    expression &expr() { return expr_; }
    std::unordered_set<std::string> &vars() { return vars_; }

    ParseResult(expression expr, std::unordered_set<std::string> vars) : expr_(std::move(expr)),
                                                                         vars_(std::move(vars)) {
    }
};

class LogicExpressionParser {
    InputCharSource source_;

    enum class TokenType { L_BRACKET, R_BRACKET, VARIABLE, AND, OR, IMPLIES, NOT, LIE };

    struct Token {
        TokenType type;
        std::string text;
    };

    class TokenBuffer {
        std::vector<Token> tokens_;
        size_t pos_ = 0;

    public:
        explicit TokenBuffer(std::vector<Token> tokens)
            : tokens_(std::move(tokens)) {
        }

        bool has_next() const { return pos_ < tokens_.size(); }
        const Token &peek() const { return tokens_[pos_]; }
        Token next() { return tokens_[pos_++]; }
    };

public:
    explicit LogicExpressionParser(InputCharSource &src)
        : source_(src) {
    }

    ParseResult parse() {
        auto tokens = tokenize();
        TokenBuffer buffer(std::move(tokens));
        if (!buffer.has_next()) {
            throw std::runtime_error("Empty input");
        }
        std::unordered_set<std::string> vars;
        auto expr = parse_impl(buffer, vars);
        if (buffer.has_next()) {
            throw std::runtime_error("Trailing garbage");
        }
        return {std::move(expr), std::move(vars)};
    }

private:
    std::vector<Token> tokenize() {
        std::vector<Token> out;
        source_.skip_ws();

        while (source_.has_next()) {
            if (const char c = source_.peek(); c == '(') {
                source_.next();
                out.push_back({TokenType::L_BRACKET, "("});
            } else if (c == ')') {
                source_.next();
                out.push_back({TokenType::R_BRACKET, ")"});
            } else if (c == '&') {
                source_.next();
                out.push_back({TokenType::AND, "&"});
            } else if (c == '|') {
                source_.next();
                out.push_back({TokenType::OR, "|"});
            } else if (c == '!') {
                source_.next();
                out.push_back({TokenType::NOT, "!"});
            } else if (c == '-') {
                source_.next();
                if (!source_.has_next() || source_.peek() != '>')
                    throw std::runtime_error("Expected >");
                source_.next();
                out.push_back({TokenType::IMPLIES, "->"});
            } else if ('A' <= c && c <= 'Z') {
                std::string name;
                name.push_back(source_.next());
                while (source_.has_next()) {
                    if (const char p = source_.peek(); 'A' <= p && p <= 'Z' || '0' <= p && p <= '9' || p == '\'') {
                        name.push_back(source_.next());
                    } else {
                        break;
                    }
                }
                out.push_back({TokenType::VARIABLE, std::move(name)});
            } else if (c == '0') {
                source_.next();
                out.push_back({TokenType::LIE, "0"});
            } else {
                throw std::runtime_error("Unexpected character");
            }

            source_.skip_ws();
        }

        return out;
    }


    expression parse_impl(TokenBuffer &buf, std::unordered_set<std::string> &vars) {
        auto left = parse_or(buf, vars);

        if (buf.has_next() && buf.peek().type == TokenType::IMPLIES) {
            buf.next();
            auto right = parse_impl(buf, vars);
            return std::make_unique<Implies>(std::move(left), std::move(right));
        }

        return left;
    }


    expression parse_or(TokenBuffer &buf, std::unordered_set<std::string> &vars) {
        auto left = parse_and(buf, vars);

        while (buf.has_next() && buf.peek().type == TokenType::OR) {
            buf.next();
            auto right = parse_and(buf, vars);
            left = std::make_unique<Or>(std::move(left), std::move(right));
        }

        return left;
    }

    expression parse_and(TokenBuffer &buf, std::unordered_set<std::string> &vars) {
        auto left = parse_unary(buf, vars);

        while (buf.has_next() && buf.peek().type == TokenType::AND) {
            buf.next();
            auto right = parse_unary(buf, vars);
            left = std::make_unique<And>(std::move(left), std::move(right));
        }

        return left;
    }

    expression parse_unary(TokenBuffer &buf, std::unordered_set<std::string> &vars) {
        if (!buf.has_next()) {
            throw std::runtime_error("Unexpected end");
        }

        if (const auto [type, name] = buf.peek(); type == TokenType::NOT) {
            buf.next();
            return std::make_unique<Not>(parse_unary(buf, vars));
        }

        return parse_primary(buf, vars);
    }

    expression parse_primary(TokenBuffer &buf, std::unordered_set<std::string> &vars) {
        if (!buf.has_next()) throw std::runtime_error("Unexpected end");

        switch (auto [type, name] = buf.next(); type) {
            case TokenType::VARIABLE: {
                vars.insert(name);
                return std::make_unique<Variable>(name);
            }

            case TokenType::LIE: {
                return std::make_unique<Lie>();
            }

            case TokenType::L_BRACKET: {
                auto inside = parse_impl(buf, vars);
                if (!buf.has_next() || buf.next().type != TokenType::R_BRACKET) {
                    throw std::runtime_error("Expected closing bracket");
                }
                return inside;
            }

            default:
                throw std::runtime_error("Unexpected token");
        }
    }
};

expression parse_str(const std::string &input) {
    std::istringstream iss(input);
    InputCharSource src(iss);
    LogicExpressionParser parser(src);
    return std::move(parser.parse().expr());
}

bool matches_impl(const LogicExpression *expr, const LogicExpression *templ,
                  std::unordered_map<std::string, std::string> &memo) {
    if (!expr || !templ) {
        return false;
    }
    if (dynamic_cast<const Variable *>(templ)) {
        const auto expr_tree = expr->to_string();
        const auto &t_name = templ->name();
        if (memo.find(t_name) != memo.end()) {
            if (memo.at(t_name) != expr_tree) {
                return false;
            }
        }
        memo[t_name] = expr_tree;
        return true;
    }
    if (expr->name() != templ->name()) {
        return false;
    }
    if (dynamic_cast<const Variable *>(expr)) {
        return dynamic_cast<const Variable *>(templ);
    }
    if (const auto *bin_e = dynamic_cast<const BinaryOperation *>(expr)) {
        if (const auto *bin_t = dynamic_cast<const BinaryOperation *>(templ)) {
            return matches_impl(bin_e->left(), bin_t->left(), memo) && matches_impl(
                       bin_e->right(), bin_t->right(), memo);
        }
    }
    if (const auto *un_e = dynamic_cast<const UnaryOperation *>(expr)) {
        if (const auto *un_t = dynamic_cast<const UnaryOperation *>(templ)) {
            return matches_impl(un_e->operand(), un_t->operand(), memo);
        }
    }
    if (dynamic_cast<const Lie *>(expr)) {
        if (dynamic_cast<const Lie *>(templ)) {
            return true;
        }
    }
    return false;
}

bool matches(const expression &expr, const expression &templ) {
    std::unordered_map<std::string, std::string> memo;
    return matches_impl(expr.get(), templ.get(), memo);
}

static const std::vector<std::pair<expression, int> > &classic_axioms() {
    static const auto &ax = []() -> const std::vector<std::pair<expression, int> > & {
        static std::vector<std::pair<expression, int> > v;
        v.reserve(10);
        v.emplace_back(parse_str("A -> B -> A"), 1);
        v.emplace_back(parse_str("(A -> B) -> (A -> B -> C) -> (A -> C)"), 2);
        v.emplace_back(parse_str("A -> B -> A & B"), 3);
        v.emplace_back(parse_str("A & B -> A"), 4);
        v.emplace_back(parse_str("A & B -> B"), 5);
        v.emplace_back(parse_str("A -> A | B"), 6);
        v.emplace_back(parse_str("B -> A | B"), 7);
        v.emplace_back(parse_str("(A -> C) -> (B -> C) -> (A | B -> C)"), 8);
        v.emplace_back(parse_str("(A -> B) -> (A -> !B) -> !A"), 9);
        v.emplace_back(parse_str("!!A -> A"), 10);
        return v;
    }();
    return ax;
}

static const std::vector<std::pair<expression, int> > &impl_axioms() {
    static const auto &ax = []() -> const std::vector<std::pair<expression, int> > & {
        static std::vector<std::pair<expression, int> > v;
        v.reserve(4);
        v.emplace_back(parse_str("A -> B -> A"), 1);
        v.emplace_back(parse_str("(A -> B) -> (A -> B -> C) -> (A -> C)"), 2);
        v.emplace_back(parse_str("A -> (A -> 0) -> B"), 3);
        v.emplace_back(parse_str("((A -> B) -> A) -> A"), 4);
        return v;
    }();
    return ax;
}

int axiom_number(const expression &expr, const std::vector<std::pair<expression, int> > &axs) {
    for (const auto &ax: axs) {
        if (matches(expr, ax.first)) {
            return ax.second;
        }
    }
    return -1;
}


std::vector<std::string_view> split(const std::string_view sv, const std::string_view delim) {
    std::vector<std::string_view> out;
    if (delim.empty()) {
        out.push_back(sv);
        return out;
    }
    size_t start = 0;
    for (;;) {
        const size_t pos = sv.find(delim, start);
        if (pos == std::string_view::npos) {
            out.push_back(sv.substr(start));
            break;
        }
        out.push_back(sv.substr(start, pos - start));
        start = pos + delim.size();
    }
    return out;
}

enum class ProofStep {
    AXIOM, HYPOTHESIS, MODUS_PONENS
};

class Annotation {
    ProofStep step_;
    int number_, info1_, info2_;

public:
    Annotation(const ProofStep step, const int number, const int info1, const int info2)
        : step_(step), number_(number), info1_(info1), info2_(info2) {
    }

    int get_number() const { return number_; }
    int get_info1() const { return info1_; }
    int get_info2() const { return info2_; }
    ProofStep get_step() const { return step_; }

    void set_number(const int number) { number_ = number; }
    void set_info1(const int info1) { info1_ = info1; }
    void set_info2(const int info2) { info2_ = info2; }

    std::string to_prefix() const {
        switch (step_) {
            case ProofStep::AXIOM: return "[" + std::to_string(number_) + ". " + "Ax. sch. " + std::to_string(info1_) +
                                          "]";
            case ProofStep::HYPOTHESIS: return "[" + std::to_string(number_) + ". " + "Hypothesis " +
                                               std::to_string(info1_) + "]";
            case ProofStep::MODUS_PONENS: return
                        "[" + std::to_string(number_) + ". " + "M.P. " + std::to_string(info1_) + ", " +
                        std::to_string(info2_) + "]";
            default: throw std::runtime_error("Unknown step");
        }
    }

    std::string to_suffix() const {
        switch (step_) {
            case ProofStep::AXIOM: return "[Ax " + std::to_string(info1_) + "]";
            case ProofStep::HYPOTHESIS: return "[Hypothesis " + std::to_string(info1_) + "]";
            case ProofStep::MODUS_PONENS: return "[MP " + std::to_string(info1_) + ", " + std::to_string(info2_) + "]";
            default: throw std::runtime_error("Unknown step");
        }
    }
};

enum class AnnotationStyle {
    PREFIX,
    SUFFIX
};

using context = std::unordered_map<std::string, int>;

std::pair<context, std::string> parse_context(std::istream &in) {
    std::string line;
    std::getline(in, line);

    auto assumptions = split(line, ",");
    const auto final = assumptions.back();
    assumptions.pop_back();

    const auto final_split = split(final, "|-");
    if (!final_split[0].empty()) {
        assumptions.push_back(final_split[0]);
    }

    std::string result = parse_str(std::string(final_split[1]))->to_string();

    context cont;
    for (int i = 0; i < assumptions.size(); ++i) {
        auto assumption_str = parse_str(std::string(assumptions[i]))->to_string();
        cont[assumption_str] = i + 1;
    }
    return {cont, result};
}

std::string annotate_proof(std::istream &in, const std::vector<std::pair<expression, int> > &axs,
                           const AnnotationStyle style, const int shift = 0) {
    auto parsed_context = parse_context(in);
    context cont = std::move(parsed_context.first);
    const std::string result = std::move(parsed_context.second);

    std::vector<std::string> proof;
    std::unordered_map<std::string, int> seen;
    std::unordered_map<std::string, std::unordered_map<std::string, int> > impl_by_suffix;
    std::vector<std::pair<int, int> > need;
    std::vector<Annotation> annotations;

    std::string input;
    while (std::getline(in, input)) {
        if (input == "$") {
            break;
        }
        proof.push_back(input);
    }

    if (proof.empty() || parse_str(proof.back())->to_string() != result) {
        return "Proof is incorrect\n";
    }

    int cur = 1;
    int final_index = -1;
    std::vector<std::string> proof2;
    for (const auto &step: proof) {
        const auto parsed = parse_str(step);
        const auto to_str = parsed->to_string();

        if (seen.find(to_str) != seen.end()) {
            continue;
        }

        need.emplace_back(-1, -1);

        if (const int ax = axiom_number(parsed, axs); ax != -1) {
            annotations.emplace_back(ProofStep::AXIOM, cur, ax, -1);
        } else if (cont.find(to_str) != cont.end()) {
            annotations.emplace_back(
                ProofStep::HYPOTHESIS, cur, cont[to_str], -1);
        } else if (impl_by_suffix.find(to_str) != impl_by_suffix.end()) {
            bool found = false;

            for (const auto &mp_map = impl_by_suffix[to_str]; const auto &pref: mp_map) {
                if (auto it = seen.find(pref.first); it != seen.end()) {
                    annotations.emplace_back(
                        ProofStep::MODUS_PONENS,
                        cur, pref.second, it->second);
                    need[cur - 1] = {pref.second - 1, it->second - 1};
                    found = true;
                    break;
                }
            }

            if (!found) {
                return "Proof is incorrect\n";
            }
        } else {
            return "Proof is incorrect\n";
        }

        proof2.push_back(step);

        if (to_str == result) {
            final_index = cur - 1;
        }

        seen[to_str] = cur;

        if (const auto *as_impl = dynamic_cast<const Implies *>(parsed.get())) {
            const auto suffix = as_impl->right()->to_string();
            const auto prefix = as_impl->left()->to_string();
            impl_by_suffix[suffix].emplace(prefix, cur);
        }

        ++cur;
    }

    std::vector need2(final_index + 1, false);
    std::vector visited(final_index + 1, false);
    need2.back() = true;

    std::function<void(int)> dfs = [&](const int v) {
        if (visited[v]) return;
        visited[v] = true;
        if (need[v].first != -1) {
            need2[need[v].first] = true;
            need2[need[v].second] = true;
            dfs(need[v].first);
            dfs(need[v].second);
        }
    };

    dfs(final_index);

    std::unordered_map<int, int> renumber;
    int idx = 1;
    for (int i = 0; i < need2.size(); ++i) {
        if (need2[i]) {
            renumber[i + 1] = idx;
            ++idx;
        }
    }

    std::string ans;
    for (const auto& elem : cont) {
        ans += elem.first + ", ";
    }
    ans.pop_back();
    ans.pop_back();
    ans += " |- " + result + "\n";

    std::function transform = [&](const int v) {
        return renumber.at(v) + shift;
    };

    for (int i = 0; i <= final_index; ++i) {
        if (need2[i]) {
            switch (annotations[i].get_step()) {
                case ProofStep::AXIOM:
                    annotations[i].set_number(transform(annotations[i].get_number()));
                    break;
                case ProofStep::HYPOTHESIS:
                    annotations[i].set_number(transform(annotations[i].get_number()));
                    break;
                case ProofStep::MODUS_PONENS:
                    annotations[i].set_number(transform(annotations[i].get_number()));
                    annotations[i].set_info1(transform(annotations[i].get_info1()));
                    annotations[i].set_info2(transform(annotations[i].get_info2()));
                    break;
                default:
                    throw std::runtime_error("Unknown step");
            }
            switch (style) {
                case AnnotationStyle::PREFIX:
                    ans += annotations[i].to_prefix();
                    ans += " ";
                    ans += proof2[i];
                    break;
                case AnnotationStyle::SUFFIX:
                    ans += proof2[i];
                    ans += " ";
                    ans += annotations[i].to_suffix();
                    break;
                default:
                    throw std::runtime_error("Unknown style");
            }
            ans += "\n";
        }
    }
    return ans;
}

void formula_analysis() {
    std::string formula;
    std::getline(std::cin, formula);
    std::istringstream iss(formula);
    InputCharSource src(iss);
    LogicExpressionParser parser(src);
    ParseResult res = parser.parse();
    const expression parsed = std::move(res.expr());
    const std::unordered_set<std::string> vars = std::move(res.vars());
    int subsets = 1 << vars.size();
    estimation estim;
    for (const auto& var : vars) {
        estim[var] = false;
    }
    for (int subset = 0; subset < subsets; ++subset) {
        auto begin = estim.begin();
        for (int i = 0; i < vars.size(); ++i) {
            begin->second = (subset & 1 << i) != 0;
            ++begin;
        }
        if (!parsed->evaluate(estim)) {
            std::cout << "Formula is refutable\n";
            for (const auto& term : estim) {
                std::cout << term.first << " = " << (term.second ? "T" : "F") << "\n";
            }
            return;
        }
    }
    std::cout << "Formula is provable\n";
}

int main() {

}