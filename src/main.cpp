#include <iostream>
#include <limits>
#include <sstream>

#include "common.h"
#include "formula.h"

int main() {
    // создаем
    auto sheet = CreateSheet();
    std::cout << "Type command count: ";
    int command_num = 0;
    std::cin >> command_num;

    while (command_num) {
        std::cout << "Specify cell(for example, A1): ";
        std::string cell_pos;
        std::cin >> cell_pos;
        std::cout << "Input cell value(for example, formula: =A1+B1 or value: 10 or text: \"any text\"): ";
        std::string str;
        std::cin >> str;
        Position p;
        sheet->SetCell(p.FromString(cell_pos), str);
        // декремент счетчика
        --command_num;
    }

    std::ostringstream ostr_value;
    // вывод вычисленных значений ячеек
    sheet->PrintValues(ostr_value);
    std::cout << "Print values: \n"
              << ostr_value.str();
    // вывод значений как есть
    std::ostringstream ostr_text;
    sheet->PrintTexts(ostr_text);
    std::cout << "Print texts: \n"
              << ostr_text.str();
}
