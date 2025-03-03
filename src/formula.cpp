#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <stdexcept>

using namespace std::literals;

std::ostream &operator<<(std::ostream &output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) try
        : ast_(ParseFormulaAST(std::move(expression))) {
    } catch (const std::runtime_error &) {
        throw;
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            return ast_.Execute(sheet);
        } catch (const FormulaError &fe) {
            return fe; // Возвращаем ошибку
        }
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> result;
        // резервируем место, чтобы не было аллокации памяти
        //result.reserve(std::distance(ast_.GetCells().begin(), ast_.GetCells().end()));
        //std::copy(ast_.GetCells().begin(), ast_.GetCells().end(), std::back_inserter(result));
        
        auto pos_list = ast_.GetCells();
        if (pos_list.empty()) {
            return {};
        }
        result.push_back(pos_list.front());
        for (const auto pos : pos_list) {
            if (result.back() == pos) {
                continue;
            }
            result.push_back(pos);
        }
        return result;
    }

private:
    FormulaAST ast_;
};
} // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}