#pragma once
#include <set>
#include <vector>
#include <map>

#include "parameter.h"

class Cloth; 

// allanyu ���ļ���ش�����δ�Ż�
class SpringsBuilder
{
public:
	// allanyu ��ͷ�ٿ�
	// void draw(Obj &cloth);

	bool build(const Cloth &cloth, unsigned int *adj_spring_st, unsigned int *adj_spring_bd);
private:
	std::vector<std::pair<unsigned int,unsigned int>> cloth_boundary_springs;   //ֻ����pair(1,2)
	std::vector<std::pair<unsigned int,unsigned int>> boundary_boundary_springs;   //Ӧ���Ѿ�����pair(1,2) && pair(2,1)
	std::set<std::pair<unsigned int,unsigned int>> boundary;
	std::vector<std::vector<unsigned int>> neigh1;   //�洢ÿ���������һ��������Ϣ(�洢�������),�� structure spring
	std::vector<std::vector<unsigned int>> neigh2;   //�洢ÿ��������ж���������Ϣ(�洢�������),�� bend spring

private:
	//void ad spring(float stiffness,Vec4s& vertices,unsigned int p1,unsigned int p2);
	bool exist(const std::vector<unsigned int>& array, const unsigned int val);
	void get_cloth_boundary_spring(const Cloth &cloth);
	void get_boundary_boundary_spring(const Cloth &cloth);
};