#pragma once
#include "d3dUtil.h"

class MaterialMap
{
public:
	void Add(const std::string& name);

private:
	std::unordered_map<std::string, std::unique_ptr<Material>> m_materials;

};

