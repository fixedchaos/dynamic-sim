#include <string>
#include <set>
#include <algorithm>
#include <iostream>
#include <cuda_runtime.h>

#include "kdtree.h"
#include "Utilities.h"
#include "Cloth.h"
#include "SpringsBuilder.h"

using namespace std;

class Matrix
{
public:
	void Insert_Matrix(unsigned int i, unsigned int j, unsigned int k, vector<pair<unsigned int, unsigned int>> &value_inline)
	{
		map<pair<unsigned int, unsigned int>, unsigned int>::iterator ite1 = mat.find(make_pair(i, j));
		map<pair<unsigned int, unsigned int>, unsigned int>::iterator ite2 = mat.find(make_pair(j, i));
		if (mat.end() != ite1)
		{
			value_inline.push_back(make_pair(ite1->second, k)); return;
		}
		if (mat.end() != ite2)
		{
			value_inline.push_back(make_pair(ite2->second, k)); return;
		}

		mat.insert(make_pair(make_pair(i, j), k));
	}
private:
	map<pair<unsigned int, unsigned int>, unsigned int>  mat;
};

///////////////////////////////
bool SpringsBuilder::exist(const vector<unsigned int>& array, const unsigned int val)
{
	if (array.end() == find(array.begin(), array.end(), val))
		return false;
	else
		return true;
}

bool SpringsBuilder::build(const Cloth &cloth, unsigned int *adj_spring_st, unsigned int *adj_spring_bd)
{
	cout << "build springs" << endl;
	if (cloth.get_obj_type() == SINGLE_LAYER_BOUNDARY)
	{
		get_cloth_boundary_spring(cloth); //������Ҫ����
		get_boundary_boundary_spring(cloth);
	}

	// ������͵���ڽӾ��󣨱䳤������Ϊ�ڽ�����
	// allanyu �㷨�����ã�Ҫ��д
	//create neigh1 for each vertex
	neigh1.resize(cloth.vertices.size());
	for (int i = 0; i < cloth.faces.size(); i++)
	{
		unsigned int f[3];
		f[0] = cloth.faces[i].v0;
		f[1] = cloth.faces[i].v1;
		f[2] = cloth.faces[i].v2;

		if (!exist(neigh1[f[0]], f[1]))   //ȥ��neighbour���ظ����ڽӵ�
			neigh1[f[0]].push_back(f[1]);
		if (!exist(neigh1[f[0]], f[2]))
			neigh1[f[0]].push_back(f[2]);

		if (!exist(neigh1[f[1]], f[0]))
			neigh1[f[1]].push_back(f[0]);
		if (!exist(neigh1[f[1]], f[2]))
			neigh1[f[1]].push_back(f[2]);

		if (!exist(neigh1[f[2]], f[0]))
			neigh1[f[2]].push_back(f[0]);
		if (!exist(neigh1[f[2]], f[1]))
			neigh1[f[2]].push_back(f[1]);
	}

	// δ���ӣ�������߽�Լ���Ķ���֮�䣬Ҳ��ɵ���
	for (int i = 0; i < cloth_boundary_springs.size(); i++)
	{
		unsigned int idx1 = cloth_boundary_springs[i].first;
		unsigned int idx2 = cloth_boundary_springs[i].second;

		neigh1[idx1].push_back(idx2);
		neigh1[idx2].push_back(idx1);
	}

	for (auto spring : boundary)
		neigh1[spring.first].push_back(spring.second);

	// allanyu����������Ҳ���������ظ���ȥ��Ӧ�����������

	//create neigh2 for each vertex
	neigh2.resize(cloth.vertices.size());
	Matrix NR;   //Neighbour Relation
	vector<pair<unsigned int, unsigned int>> point_inline;  //�洢�������������ζԽǶ�������

	// �ҳ����ߵ����������εĲ����������������
	// ͨ�����ΰ�ÿ������Ϊkey��ʣ���Ǹ�������value������һ��map
	// ���map������һ��������key���Ǹ�value�͵�ǰ��value����һ�����������Ķ����
	// ���Ӷ�O(f lg e)
	// ���⻹��һ��ʵ�֣�ͨ���ڽ����ң��㷨����ͬ�Ҷ����һ���ھӣ������Ӷ�O(f)
	for (int i = 0; i < cloth.faces.size(); i++)
	{
		unsigned int f[3];
		f[0] = cloth.faces[i].v0;
		f[1] = cloth.faces[i].v1;
		f[2] = cloth.faces[i].v2;

		NR.Insert_Matrix(f[0], f[1], f[2], point_inline);
		NR.Insert_Matrix(f[0], f[2], f[1], point_inline);
		NR.Insert_Matrix(f[1], f[2], f[0], point_inline);
	}

	for (int i = 0; i < point_inline.size(); i++)
	{
		unsigned int fir = point_inline[i].first;
		unsigned int sec = point_inline[i].second;

		neigh2[fir].push_back(sec);
		neigh2[sec].push_back(fir);
	}
	cout << "springs build successfully!" << endl;

	vector<unsigned int> cpu_neigh1(neigh1.size() * sim_parameter.NUM_PER_VERTEX_SPRING_STRUCT, 0);
	vector<unsigned int> cpu_neigh2(neigh2.size() * sim_parameter.NUM_PER_VERTEX_SPRING_BEND, 0);

	for (int i = 0; i < neigh1.size(); i++)
	{
		unsigned int j;
		for (j = 0; j < neigh1[i].size() && j < sim_parameter.NUM_PER_VERTEX_SPRING_STRUCT; j++)
		{
			cpu_neigh1[i*sim_parameter.NUM_PER_VERTEX_SPRING_STRUCT + j] = neigh1[i][j];
		}
		if (sim_parameter.NUM_PER_VERTEX_SPRING_STRUCT > neigh1[i].size())
			cpu_neigh1[i*sim_parameter.NUM_PER_VERTEX_SPRING_STRUCT + j] = UINT_MAX;     //sentinel

	}

	for (int i = 0; i < neigh2.size(); i++)
	{
		unsigned int j;
		for (j = 0; j < neigh2[i].size() && j < sim_parameter.NUM_PER_VERTEX_SPRING_BEND; j++)
		{
			cpu_neigh2[i*sim_parameter.NUM_PER_VERTEX_SPRING_BEND + j] = neigh2[i][j];
		}
		if (sim_parameter.NUM_PER_VERTEX_SPRING_BEND > neigh2[i].size())
			cpu_neigh2[i*sim_parameter.NUM_PER_VERTEX_SPRING_BEND + j] = UINT_MAX;     //sentinel
	}

	safe_cuda(cudaMemcpy(adj_spring_st, &cpu_neigh1[0], cpu_neigh1.size() * sizeof(unsigned int), cudaMemcpyHostToDevice));
	safe_cuda(cudaMemcpy(adj_spring_bd, &cpu_neigh2[0], cpu_neigh2.size() * sizeof(unsigned int), cudaMemcpyHostToDevice));
	return true;
}

void SpringsBuilder::get_cloth_boundary_spring(const Cloth &cloth)
{
	//��һ�ν�����֮��Ӧ�ñ��������Ϣ���ı�����֡simulationʱʹ�õ�һ�ε�����
	//����˳��Ϊcloth1,cloth1_boundary,cloth2,cloth2_boundary...

	int g_start = 0;
	unsigned int *idx = new unsigned int[cloth.vertices.size()];
	for (int n = 0; n < cloth.objects.size(); n += 2)
	{
		unsigned int group_size = cloth.objects[n].second;
		kdtree *kd = kd_create(3);

		for (unsigned int i = 0; i < group_size; i++)   //Ϊ��Ƭ1����kdtree
		{
			idx[i + g_start] = i + g_start;
			int ret = kd_insert3f(kd, cloth.vertices[i + g_start].x,
				cloth.vertices[i + g_start].y,
				cloth.vertices[i + g_start].z,
				&idx[i + g_start]);
		}
		g_start += cloth.objects[n].second;

		for (unsigned int i = 0; i < cloth.objects[n + 1].second; i++)    //Ϊ�߽��еĵ������ڽ���
		{
			float kdpos[3];
			kdres *result = kd_nearest3f(kd, cloth.vertices[i + g_start].x,
				cloth.vertices[i + g_start].y,
				cloth.vertices[i + g_start].z);
			int *resultidx = (int*)kd_res_itemf(result, kdpos);
			cloth_boundary_springs.push_back(make_pair(i + g_start, *resultidx));
		}
		g_start += cloth.objects[n + 1].second;

		kd_free(kd);
	}
	delete[]idx;

}

void SpringsBuilder::get_boundary_boundary_spring(const Cloth &cloth)
{
	//���ѡȡN����Ϊ�߽�����Ƭ���ڵ�֮���������
	float max_dist = 0;
	const unsigned int NUM = 100;
	for (int i = 0; i < NUM; i++)
	{
		unsigned int idx1 = cloth_boundary_springs[i].first;
		unsigned int idx2 = cloth_boundary_springs[i].second;
		std::cout << idx1 << " " << idx2 << std::endl; 
		max_dist += glm::distance(cloth.vertices[idx1], cloth.vertices[idx2]);
	}
	max_dist /= NUM;
	cout << "�߽�����Ƭ���ڵ�֮��������룺" << max_dist << endl;

	////Ϊ�߽�֮�佨������:�̶�һ�飬��ʣ��boundary��������
	vector<pair<unsigned int, unsigned int>> start_end;
	int start = 0;
	for (int n = 0; n < cloth.objects.size(); n += 2)
	{
		start += cloth.objects[n].second;
		start_end.push_back(make_pair(start, start + cloth.objects[n + 1].second));
		start += cloth.objects[n + 1].second;
	}

	int *idx = new int[cloth.vertices.size()];
	for (int i = 0; i < start_end.size(); i++)
	{
		//��ǰ����Ϊ��iƬboundary
		//Ϊ��i�����������boundary����kdtree
		kdtree *kd = kd_create(3);
		for (int j = 0; j < start_end.size(); j++)
		{
			if (j == i) continue;
			for (unsigned int k = start_end[j].first; k < start_end[j].second; k++)
			{
				idx[k] = k;
				int ret = kd_insert3f(kd, cloth.vertices[k].x,
					cloth.vertices[k].y,
					cloth.vertices[k].z,
					&idx[k]);
			}
		}

		//��ʼ��������������
		for (unsigned int k = start_end[i].first; k < start_end[i].second; k++)
		{
			float kdpos[3];
			kdres *result = kd_nearest3f(kd, cloth.vertices[k].x,
				cloth.vertices[k].y,
				cloth.vertices[k].z);
			int *resultidx = (int*)kd_res_itemf(result, kdpos);

			if (glm::distance(cloth.vertices[k], cloth.vertices[*resultidx]) < max_dist * 50
				&& glm::distance(cloth.vertices[k], cloth.vertices[*resultidx]) > 0) //��������жϣ���ֹ����
			{
				boundary_boundary_springs.push_back(make_pair(k, *resultidx));
			}
		}

		kd_free(kd);
	}
	delete[]idx;


	//map[boundary,cloth]
	map<unsigned int, unsigned int> map_spring;
	for (auto spring : cloth_boundary_springs)
		map_spring[spring.first] = spring.second;

	for (auto spring : boundary_boundary_springs)
	{
		boundary.insert(make_pair(map_spring[spring.first], map_spring[spring.second]));
	}








	////FR->_Piece2
	//vector<pair<unsigned int,unsigned int>> start_end;
	//int start = 0;
	//for(int n=0;n<cloth.objects.size();n++)
	//{
	//	start_end.push_back(make_pair(start,start+cloth.objects[n].second));
	//	start += cloth.objects[n].second;
	//}
	//unsigned int *idx = new unsigned int[cloth.vertices.size()]; 
	//kdtree *kd = k create(3);
	//for (int i=start_end[8].first;i<start_end[8].second;i++)   //Ϊ��Ƭ1����kdtree
	//	{
	//		idx[i] = i;
	//		int ret = k insert3f(kd, cloth.vertices[i].x,
	//			cloth.vertices[i].y,
	//			cloth.vertices[i].z,
	//			&idx[i]);
	//	}

	//for(int i=start_end[1].first;i<start_end[1].second;i++)
	//{
	//	float kdpos[3];
	//		kdres *result = k nearest3f(kd, cloth.vertices[i].x,
	//			cloth.vertices[i].y,
	//			cloth.vertices[i].z);
	//		int *resultidx = (int*)k res_itemf(result, kdpos);

	//		if (glm::distance(cloth.vertices[i],cloth.vertices[*resultidx]) < max_dist*20) //��������жϣ���ֹ����
	//		{
	//			boundary_boundary_springs.push_back(make_pair(i,*resultidx));
	//		}
	//}


}

#if 0
void SpringsBuilder::draw(Obj &cloth)
{
	for (int i = 0; i < neigh1.size(); i++)
	{
		glm::vec4 v1 = cloth.vertices[i];
		for (int j = 0; j < neigh1[i].size(); j++)
		{
			glm::vec4 v2 = cloth.vertices[neigh1[i][j]];

			glBegin(GL_LINES);
			glColor3f(1.0, 1.0, 1.0);
			glVertex3f(v1.x, v1.y, v1.z);
			glVertex3f(v2.x, v2.y, v2.z);
			glEnd();
		}
	}

	for (int i = 0; i < neigh2.size(); i++)
	{
		glm::vec4 v1 = cloth.vertices[i];
		for (int j = 0; j < neigh2[i].size(); j++)
		{
			glm::vec4 v2 = cloth.vertices[neigh2[i][j]];
			glBegin(GL_LINES);
			glColor3f(1.0, 0, 0);
			glVertex3f(v1.x, v1.y, v1.z);
			glVertex3f(v2.x, v2.y, v2.z);
			glEnd();
		}
	}
}

#endif