#pragma once
#include <vector>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>

#include <glm/glm.hpp>

#include "SpringsBuilder.h"
#include "bvh\BVHAccel.h"
#include "bvh\BVHBuilder.h"

class Cloth;
class Mesh;
class BRTreeNode;
class Primitive;

class Simulator : public RefObject
{
public:
	// ���ڸ���Simulation�õ�����
	Simulator();
	~Simulator();

	// һ�����������̬��ײ
	void simulate();
	// �������λ��ʱ��ccd��ײ
	void ccd();

	// ��ʾ���
	void visulize();

	// debug
	void draw_collided_vertex();

	// ��ʼ����ģ��ʱ��Ҫ�õ��Ĳ����ռ䣬��ģ�������ģ�йأ��������޹�
	void update_cloth(const Cloth &cloth);
	void update_body(const Mesh &body);

private:
	virtual void ref_auto_clean();

	void create_buffer();

	// ��̬��ײ�����㷨
	void verlet_cuda();

	// ��̬��ײ�����㷨
	void ccd_cuda();

	void computeGridSize(unsigned int n, unsigned int block_size, unsigned int &num_blocks, unsigned int &num_threads)
	{
		num_threads = std::min(block_size, n);
		num_blocks = (n % num_threads != 0) ? (n / num_threads + 1) : (n / num_threads);
	}

	// �������롢����ռ�
	// ÿ����������ǰλ�á���һ��λ��
	// ������������������2 * 2 = 4 ��
	// �����ڼ�������У�����ײ���ܵ�����һ�ε�pose_last != ��һ�ε�pose
	// �粻��pose_last���ݣ����ռ�洢������ڼ���ʱ��ֱ��Ӱ����һ�ε�pose
	// ����һ�ε�pose���ᱻ�����̼߳��������ڵ�ʱ�õ�
	void swap_buffer();

private:
	//////////////////bvh����///////////////////
	BVHBuilder _bvh_builder;
	BVHBuilder _ccd_builder;
	BVHAccel _bvh_tree;
	BVHAccel _ccd_tree;

	//////////////////cloth����///////////////////
	SpringsBuilder _springs_builder;

	// ��ǰbuffer����
	unsigned int _size_faces;
	unsigned int _size_vertices;

	// ��ǰʵ������
	unsigned int _num_faces;
	unsigned int _num_vertices;
	unsigned int _num_textures;

	// һ������buffer��ÿ��ֱ�洢�˵�ǰλ�ú�ǰһ��λ�ã��������������롢���
	// ��id�����swap_buffer�������Ӷ�ʵ�����롢�����Ļ���
	int _id_in, _id_out;

	// ��������ȫΪ�Դ�ָ��
private:
	// ����ԭ�������ﲻ���������Զ����
	glm::vec3 *_d_x_orignal;

	// ÿ�ε������������ݺ��������ָ�룬�ֵ�ǰֵ����һ��ֵ
	// ��ʵ�ʿռ䣬ͨ��ָ��_d_x_cur��_d_x_lst�õ���ַ
	// ͨ������ָ��ʵ��������������ֻ�
	glm::vec3 *_d_x_cur_in, *_d_x_cur_out;
	glm::vec3 *_d_x_lst_in, *_d_x_lst_out;

	// ʵ�����ڴ洢�Ŀռ䣬_d_x_cur_in��ָ������
	glm::vec3 *_d_x_cur[2];
	glm::vec3 *_d_x_lst[2];

	glm::vec3 *_d_dir_collision_force;		// ��ײ�����������εķ����������û����ײ��Ϊ0
	glm::vec3 *_d_dir_face_normals;		// ��ķ�����

	// ������-���㣬�ڽӾ���
	// ������Ƭ��Ӧ�����������ÿ��������Ƭ����������
	// ��i��������ĵ�j�����㣬λ��Ϊi * 3 + j
	unsigned int *_d_adj_face_to_vertices;

	// ����-�����Σ��ڽӾ���
	// ÿ�������Ӧ��������Ƭ������ÿ�������Ӧ�����θ�������
	// ͳһ����NUM_PER_VERTEX_ADJ_FACES���ڽ�������ռ�
	// ������UINT_MAX
	// ����ÿ���㷨����ʱ��Ҫ����Χƽ�������
	unsigned int *_d_adj_vertex_to_faces;

	// ����-���㣬�ڽӾ���
	// ��ע�⣩һ������ + ����߽�Լ���Ĳ����ӵ�
	// ��i������ĵ�j���ھӣ�λ��Ϊi * NUM_PER_VERTEX_SPRING_STRUCT + j
	// ������UINT_MAX
	unsigned int *_d_adj_spring_st;

	// ����-���㣬�ڽӾ���
	// ���������β���������������㣬��Ϊ��������
	// ��i������ĵ�j���ھӣ�λ��Ϊi * NUM_PER_VERTEX_SPRING_BEND + j
	// ������UINT_MAX
	unsigned int *_d_adj_spring_bd;

	// ���ڹ���������Ƶ�opengl��Դ��
	// ͨ��cuda���룬ֱ����gpu�и�д����������ʾ������
	cudaGraphicsResource *_d_vbo_resource;
	glm::vec4 *_d_vbo_vertices;           // ָ��OPENGL buffer��vertex�ĵ�ַ
	glm::vec3 *_d_vbo_normals;            // ָ��OPENGL buffer��normal�ĵ�ַ


// ���ڽ������·��ļ��ֵ��ɣ��������Ϊ_d_adj_spring_st��_d_adj_spring_bd
	// ���ı�Ϊ�ֲ������������·�ʱʹ�ã������õ��ɺ�����
		// Springs *cuda_spring;

#ifdef DEBUG_COLLISION
	// debug
	int *collided_vertex;
	std::vector<int> cpu_collided_veretx;
	Vec4s updated_vertex;
	vector<Face> faces;
#endif

};

