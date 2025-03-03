#include "cell.h"

#include <cassert>
#include <iostream>
#include <optional>
#include <queue>
#include <string>
#include <unordered_set>

Cell::Cell(Sheet &sheet, Position pos) : impl_(nullptr), cur_pos_(pos), sheet_(sheet) {};

Cell::~Cell() {
    impl_.reset();
}

void Cell::Set(std::string text) {
    if (text[0] == FORMULA_SIGN && text.size() > 1) {
        std::string value = "=";
        std::unique_ptr<FormulaInterface> formula;
        try {
            formula = ParseFormula(text.substr(1));
        } catch (const std::exception &) {
            throw FormulaException("Incorrect Position");
        }
        auto expression = formula->GetExpression();
        if (!IsValidReferences(formula->GetReferencedCells())) {
            throw FormulaException("Invalid position");
        }
        referenced_cells_ = formula->GetReferencedCells();
        // проверка на цикл
        if (IsReferenced()) {
            throw CircularDependencyException("Cycle detected");
        }
        FillDependCells();
        value += expression;
        impl_ = std::make_unique<FormulaImpl>(value);
    } else if (text[0] == ESCAPE_SIGN || text.size() > 0) {
        impl_ = std::make_unique<TextImpl>(text);
    } else {
        impl_ = std::make_unique<EmptyImpl>();
    }
}

void Cell::Clear() {
    Set("");
}

Cell::Value Cell::GetValue() const {
    if (cache_value_.has_value()) {
        Cell::Value res;
        if (std::holds_alternative<double>(cache_value_.value())) {
            res = std::get<double>(cache_value_.value());
        } else if (std::holds_alternative<FormulaError>(cache_value_.value())) {
            res = std::get<FormulaError>(cache_value_.value());
        }
        return res;
    }
    auto str = impl_->GetValue();
    if (str[0] == FORMULA_SIGN && str.size() > 1) {
        auto eval = ParseFormula(str.substr(1))->Evaluate(sheet_);
        if (std::holds_alternative<double>(eval)) {
            cache_value_ = std::get<double>(eval);
            return std::get<double>(eval);
        }
        cache_value_ = std::get<FormulaError>(eval);
        return std::get<FormulaError>(eval);
    } else if (str[0] == ESCAPE_SIGN) {
        return str.substr(1);
    }

    // Попробуем преобразовать строку в double
    if (str == "") {
        return 0.;
    }
    return str;
}

std::string Cell::GetText() const {
    if (impl_ == nullptr) {
        return "null";
    }
    return impl_->GetValue();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return referenced_cells_;
}

void Cell::EraseCache() {
    cache_value_.reset();
}

void Cell::CacheInvalidator(/* const std::vector<Position> &ptr_from */) {
    auto &sheet = sheet_.GetCellsSet();
    for (auto cell : depend_cells_) {
        sheet[cell.row][cell.col].get()->CacheInvalidator();
        sheet[cell.row][cell.col].get()->EraseCache();
    }
    depend_cells_.clear();
}

void Cell::FillDependCells() {
    auto &sheet = sheet_.GetCellsSet();
    for (auto &cell : referenced_cells_) {
        if (cell == cur_pos_) {
            continue;
        }
        if (!sheet_.IsDataExist(cell)) {
            sheet_.SetCell(cell, "");
        }
        auto depend_cell = sheet[cell.row][cell.col].get();
        depend_cell->AppendDependCell(cur_pos_);
    }
}

void Cell::AppendDependCell(Position pos) {
    depend_cells_.emplace_back(pos);
}

bool Cell::IsValidReferences(const std::vector<Position> ref_pos) const {
    for (const auto pos : ref_pos) {
        if (!pos.IsValid()) {
            return false;
        }
    }
    return true;
}

struct PositionHash {
    std::size_t operator()(const Position &pos) const {
        return std::hash<int>()(pos.row) ^ std::hash<int>()(pos.col);
    }
};

bool Cell::IsReferenced() const {
    std::queue<Position> queue;
    std::unordered_set<Position, PositionHash> visited;

    queue.push(cur_pos_);
    visited.insert(cur_pos_);

    while (!queue.empty()) {
        Position current = queue.front();
        queue.pop();
        // Перебираем все смежные вершины -
        if (!sheet_.IsDataExist(current)) {
            continue;
        }
        for (const Position &cell : sheet_.GetCell(current)->GetReferencedCells()) {
            // если узел не встречался - петли нет
            if (visited.find(cell) == visited.end()) {
                visited.insert(cell);
                queue.push(cell);
            } else {
                return true;
            }
        }
    }
    return false;
}