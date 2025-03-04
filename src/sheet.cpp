#include "sheet.h"

#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    ValidationForException(pos);

    // Сохраняем старое значение ячейки (если оно существует)
    std::unique_ptr<Cell> old_cell;
    if (IsDataExist(pos)) {
        cells_[pos.row][pos.col].get()->CacheInvalidator();
        old_cell = std::move(cells_[pos.row][pos.col]);
    }
    if (pos.row >= static_cast<int>(cells_.size())) {
        cells_.resize(pos.row + 1);
    }
    if (pos.col >= static_cast<int>(cells_.at(pos.row).size())) {
        cells_.at(pos.row).resize(pos.col + 1);
    }
    // Создаем новую ячейку и помещаем ее в таблицу

    auto new_cell = std::make_unique<Cell>(*this, pos);
    cells_[pos.row][pos.col] = std::move(new_cell);
    try {
        // Пытаемся установить значение в ячейку
        cells_[pos.row][pos.col]->Set(std::move(text));
    } catch (const CircularDependencyException &) {
        // Восстанавливаем старое значение ячейки
        cells_[pos.row][pos.col] = std::move(old_cell);
        throw; // Перебрасываем исключение
    } catch (const FormulaException &) {
        // Восстанавливаем старое значение ячейки
        cells_[pos.row][pos.col] = std::move(old_cell);
        throw; // Перебрасываем исключение
    }
    Sheet::PrintableSizeIncrease(pos);
}

const CellInterface *Sheet::GetCell(Position pos) const {
    ValidationForException(pos);

    if (Sheet::IsDataExist(pos)) {
        return cells_.at(pos.row).at(pos.col).get();
    }
    return nullptr;
}

CellInterface *Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }
    if (Sheet::IsDataExist(pos)) {
        return cells_.at(pos.row).at(pos.col).get();
    }
    return nullptr;
}

void Sheet::ClearCell(Position pos) {
    ValidationForException(pos);

    if (Sheet::IsDataExist(pos)) {
        cells_[pos.row][pos.col].release();
    }
    Sheet::PrintableSizeDecrease(/* pos */);
}

Size Sheet::GetPrintableSize() const {
    return print_size_;
}

void Sheet::PrintValues(std::ostream &output) const {
    for (int row = 0; row < print_size_.rows; ++row) {
        for (int col = 0; col < print_size_.cols; ++col) {
            if (Sheet::IsDataExist(Position{row, col})) {
                ConvertVariantOutputData(output, cells_.at(row).at(col).get()->GetValue());
            }
            if (print_size_.cols > col + 1)
                output << '\t';
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream &output) const {
    for (int row = 0; row < print_size_.rows; ++row) {
        for (int col = 0; col < print_size_.cols; ++col) {
            if (Sheet::IsDataExist(Position{row, col})) {
                output << cells_.at(row).at(col).get()->GetText();
            }
            if (print_size_.cols > col + 1)
                output << '\t';
        }
        output << '\n';
    }
}

// обновление печатной области после вставки данных в ячейку
void Sheet::PrintableSizeIncrease(Position pos) {
    // новая позиция колонки больше текущей максимальной
    if (pos.col + 1 > print_size_.cols) {
        print_size_.cols = pos.col + 1;
    }
    // новая позиция строки больше текущей максимальной
    if (pos.row + 1 > print_size_.rows) {
        print_size_.rows = pos.row + 1;
    }
}

// обновление печатной области после очистки данных в ячейке
void Sheet::PrintableSizeDecrease(/* Position pos */) {
    int min_y = 0;
    int min_x = 0;
    for (size_t i = 0; i < cells_.size(); ++i) {
        for (size_t j = 0; j < cells_.at(i).size(); ++j) {
            if (cells_.at(i).at(j) == nullptr) {
                continue;
            }
            if (static_cast<int>(i) + 1 > min_y) {
                min_y = static_cast<int>(i) + 1;
            }
            if (static_cast<int>(j) + 1 > min_x) {
                min_x = static_cast<int>(i) + 1;
            }
        }
    }
    print_size_.rows = min_y;
    print_size_.cols = min_x;
}

void Sheet::ConvertVariantOutputData(std::ostream &output, const CellInterface::Value &value) const {
    std::visit(Overloads{
                   [&output](const std::string &str) { output << str; },
                   [&output](const double d) { output << d; },
                   [&output](const FormulaError &fe) { output << fe; }},
               value);
}

bool Sheet::IsDataExist(Position pos) const {
    return pos.row < static_cast<int>(cells_.size()) && pos.col < static_cast<int>(cells_.at(pos.row).size()) && cells_.at(pos.row).at(pos.col).get() != nullptr;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

const CellsSet &Sheet::GetCellsSet() const {
    return cells_;
}

void Sheet::ValidationForException(const Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }
}