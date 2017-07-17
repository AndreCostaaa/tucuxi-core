/*
* Copyright (C) 2017 Tucuxi SA
*/

#include "rapidxml.hpp"

#include "xmlnode.h"
#include "xmlattribute.h"

namespace Tucuxi {
namespace Common {

XmlAttribute::XmlAttribute()
    : m_pAttribute(nullptr)
{
}


XmlAttribute::XmlAttribute(const XmlAttribute& _node)
    :m_pAttribute(_node.m_pAttribute)
{
}


XmlAttribute::XmlAttribute(rapidxml::xml_attribute<>* _pAttribute)
{
    m_pAttribute = _pAttribute;
}


XmlAttribute::~XmlAttribute()
{
}


XmlNode XmlAttribute::getNode()
{
    rapidxml::xml_node<>* pNode = nullptr;
    if (m_pAttribute != nullptr) {
        pNode = m_pAttribute->parent();
    }
    return XmlNode(pNode);
}


std::string XmlAttribute::getName() const
{
    if (m_pAttribute != nullptr) {
        return m_pAttribute->name();
    }
    return "";
}


void XmlAttribute::setName(const std::string& _name)
{
    char *pstr = allocateString(_name);
    if (pstr != nullptr) {
        m_pAttribute->name(pstr);
    }
}


std::string XmlAttribute::getValue() const
{
    if (m_pAttribute != nullptr) {
        return m_pAttribute->value();
    }
    return "";
}


void XmlAttribute::setValue(const std::string& _value)
{
    char *pstr = allocateString(_value);
    if (pstr != nullptr) {
        m_pAttribute->value(pstr);
    }
}


XmlAttribute& XmlAttribute::operator=(const XmlAttribute& _other)
{
    m_pAttribute = _other.m_pAttribute;
    return *this;
}


bool XmlAttribute::operator==(const XmlAttribute& _other) const
{
    return m_pAttribute == _other.m_pAttribute;
}


bool XmlAttribute::operator!=(const XmlAttribute& _other) const
{
    return m_pAttribute != _other.m_pAttribute;
}


XmlAttribute XmlAttribute::next() const
{
    rapidxml::xml_attribute<>* pAttribute = nullptr;
    if (m_pAttribute != nullptr) {
        pAttribute = m_pAttribute->next_attribute();
    }
    return XmlAttribute(pAttribute);
}


char* XmlAttribute::allocateString(const std::string& _string)
{
    if (m_pAttribute != nullptr) {
        try {
            return m_pAttribute->document()->allocate_string(_string.c_str());
        }
        catch (std::bad_alloc) {
        }
    }
    return nullptr;
}

}
}
