/*
* Copyright (C) 2017 Tucuxi SA
*/

#ifndef TUCUXI_CORE_PARAMTERIDS_H
#define TUCUXI_CORE_PARAMTERIDS_H

#include <map>
#include <string>

namespace Tucuxi {
namespace Core {

class ParameterId 
{
public:
    static const int size = 17;
    enum Enum { V, V1, V2, CL, Ka, Ke, K12, K21, K13, K31, F, Q, Q1, Q2, a, b, Unknown };

    static Enum fromString(const std::string &_id) {
        static const std::map<std::string, Enum> paramIds = {
            { "V",   ParameterId::V },
            { "V1",  ParameterId::V1 },
            { "V2",  ParameterId::V2 },
            { "CL",  ParameterId::CL },
            { "Ka",  ParameterId::Ka },
            { "Ke",  ParameterId::Ke },
            { "K12", ParameterId::K12 },
            { "K21", ParameterId::K21 },
            { "K13", ParameterId::K13 },
            { "K31", ParameterId::K31 },
            { "F",   ParameterId::F },
            { "Q",   ParameterId::Q },
            { "Q1",  ParameterId::Q1 },
            { "Q2",  ParameterId::Q2 },
            { "a",  ParameterId::a },
            { "b",  ParameterId::b }
        };
        std::map<std::string, Enum>::const_iterator it = paramIds.find(_id);
        if (it != paramIds.end()) {
            return it->second;
        }
        return ParameterId::Unknown;
    }
};

} // namespace Core
} // namespace Tucuxi

#endif // TUCUXI_CORE_PARAMTERIDS_H
