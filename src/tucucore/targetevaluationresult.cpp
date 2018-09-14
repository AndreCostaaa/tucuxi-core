#include "targetevaluationresult.h"

namespace Tucuxi {
namespace Core {

TargetEvaluationResult::TargetEvaluationResult(TargetType _targetType, double _score, Value _value) :
    m_targetType(_targetType), m_score(_score), m_value(_value)
{

}


} // namespace Core
} // namespace Tucuxi