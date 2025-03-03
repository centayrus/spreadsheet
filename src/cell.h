#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <memory>
#include <optional>
#include <string>

class Impl {
public:
    Impl() = default;

    virtual ~Impl() = default;

    virtual std::string GetValue() const = 0;
};

class EmptyImpl : public Impl {
public:
    EmptyImpl() = default;

    ~EmptyImpl() override = default;

    std::string GetValue() const override {
        return empty_string_;
    }

private:
    std::string empty_string_ = "";
};

class TextImpl : public Impl {
public:
    TextImpl(std::string text) : str_(std::move(text)) {
    }

    ~TextImpl() override = default;

    std::string GetValue() const override {
        return str_;
    }

private:
    std::string str_;
};

class FormulaImpl : public Impl {
public:
    FormulaImpl(std::string text) : formula_(std::move(text)) {
    }

    ~FormulaImpl() override = default;

    std::string GetValue() const override {
        return formula_;
    }

private:
    std::string formula_;
};

class Sheet;

class Cell : public CellInterface {
public:
    explicit Cell(Sheet &sheet, Position pos);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    // дополняет depend_cells_ ячеек, от которой зависит текущая
    void FillDependCells();

    void AppendDependCell(Position pos);

    // внутренний метод для инвалидации кеша ячеек связанных с текущей
    // вызывается в Sheet::SetCell для непустой ячейки
    void CacheInvalidator(/* const std::vector<Position> &ptr_from */);

private:
    std::unique_ptr<Impl> impl_ = nullptr;
    Position cur_pos_;
    mutable std::optional<FormulaInterface::Value> cache_value_; // кеш устанавливается в методе GetValue()
    Sheet &sheet_;
    std::vector<Position> referenced_cells_; // позиции ячеек, на которые ссылается текущая,
                                             // заполняется в методе Set в лексическом анализаторе
    std::vector<Position> depend_cells_;     // * это ячейки, которые ссылаются на текущую, заполняются в методе GetValue()
                                             // позиции ячеек, чей кеш нужно инвалидировать, т.к. текщая ячейка изменила значение

    bool IsValidReferences(const std::vector<Position> ref_pos) const;

    // чистит кеш в конкретной ячейке
    void EraseCache();

    bool IsReferenced() const;
};