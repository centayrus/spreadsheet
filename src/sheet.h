#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <memory>
#include <variant>
#include <vector>

class Cell;

using CellsSet = std::vector<std::vector<std::unique_ptr<Cell>>>;

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface *GetCell(Position pos) const override;
    CellInterface *GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream &output) const override;
    void PrintTexts(std::ostream &output) const override;

    const CellsSet &GetCellsSet() const;

    bool IsDataExist(Position pos) const;

private:
    CellsSet cells_;
    Size print_size_;

    // обновление печатной области после вставки данных в ячейку
    void PrintableSizeIncrease(Position pos);

    // обновление печатной области после очистки данных в ячейке
    void PrintableSizeDecrease(/* Position pos */);

    // Перегрузки для std::visit
    template <class... Ts>
    struct Overloads : Ts... {
        using Ts::operator()...;
    };
    template <class... Ts>
    Overloads(Ts...) -> Overloads<Ts...>;

    void ConvertVariantOutputData(std::ostream &output, const CellInterface::Value &value) const;

    void ValidationForException(const Position) const;
};