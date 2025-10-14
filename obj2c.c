#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Limits
//Aproximate memory usage by max sizes:
// 512 Everything max: 52kb
// 1024 Everything max: 104kb
// 2048 Everything max: 208kb
// 4096 Everything max: 416kb
#define MAX_VERTICES 2048
#define MAX_TEXCOORDS 2048
#define MAX_FACES 2048
#define MAX_EXPANDED (MAX_FACES * 3)

//Data structures for OBJ
typedef struct { float x, y, z; } Vec3;
typedef struct { float u, v; } Vec2;
typedef struct { int v[3], vt[3]; } Face;

Vec3 vertices[MAX_VERTICES];
Vec2 texcoords[MAX_TEXCOORDS];
Face faces[MAX_FACES];
int vert_count = 0, tex_count = 0, face_count = 0;

// _CRT_SECURE_NO_WARNINGS // Put this in project settings if you want to avoid warnings

// Struct for expanded vertex with UV
typedef struct {
	Vec3 pos;
	Vec2 uv;
} VertexUV;

// Compare functions
static bool cmp_vec3(const Vec3* a, const Vec3* b) {
	return (a->x == b->x) && (a->y == b->y) && (a->z == b->z);
}
static bool cmp_vec2(const Vec2* a, const Vec2* b) {
	return (a->u == b->u) && (a->v == b->v);
}

// Function to find or add a vertex with UV
int find_or_add_vertex(VertexUV* expanded, int* expanded_count, Vec3 pos, Vec2 uv)
{
	for (int i = 0; i < *expanded_count; i++) {
		if (cmp_vec3(&expanded[i].pos, &pos) && cmp_vec2(&expanded[i].uv, &uv))
		{
			return i;
		}
	}

	//Add new vertex
	expanded[*expanded_count].pos = pos;
	expanded[*expanded_count].uv = uv;
	return (*expanded_count)++;
}

int export_c(const char* outname)
{
	//Create output file
	FILE* outFile = fopen(outname, "w");
	if (outFile == NULL)
	{
		perror("Error creating output file!\n");
		return 0;
	}

	// Expand vertices with UVs
	VertexUV expanded[MAX_EXPANDED];
	int expanded_count = 0;
	int expanded_indices[MAX_FACES][3]; // Store indices for each face

	// For each face, for each vertex, find or add the vertex+uv combination
	for (int f = 0; f < face_count; f++) {
		for (int v = 0; v < 3; v++) {
			int vi = faces[f].v[v];
			int ti = faces[f].vt[v];
			Vec3 pos = vertices[vi];
			Vec2 uv = texcoords[ti];
			uv.v = 1.0f - uv.v; // Flip V coordinate for some systems (PS2SDK for example)
			int idx = find_or_add_vertex(expanded, &expanded_count, pos, uv);
			expanded_indices[f][v] = idx;
		}
	}

	// Write vertices
	fprintf(outFile, "int vertex_count = %d;\n", expanded_count);
	fprintf(outFile, "VECTOR vertices[%d] = {\n", expanded_count);
	for (int i = 0; i < expanded_count; i++)
	{
		fprintf(outFile, "  { %.2ff, %.2ff, %.2ff, 1.00f }%s\n",
			expanded[i].pos.x, expanded[i].pos.y, expanded[i].pos.z,
			(i + 1 == expanded_count) ? "" : ",");
	}
	fprintf(outFile, "};\n\n");

	// Write faces (points)
	fprintf(outFile, "int points_count = %d;\n", face_count * 3);
	fprintf(outFile, "int points[%d] = {\n", face_count * 3);
	for (int j = 0; j < face_count; j++)
	{
		fprintf(outFile, "  %d, %d, %d%s\n",
			expanded_indices[j][0], expanded_indices[j][1], expanded_indices[j][2],
			j + 1 == face_count ? "" : ",");
	}
	fprintf(outFile, "};\n\n");

	// Write UV coordinates
	fprintf(outFile, "VECTOR coordinates[%d] = {\n", expanded_count);
	for (int i = 0; i < expanded_count; i++)
	{
		fprintf(outFile, "  { %.6ff, %.6ff, 0.00f, 0.00f }%s\n",
			expanded[i].uv.u, expanded[i].uv.v,
			(i + 1 == expanded_count) ? "" : ",");
	}
	fprintf(outFile, "};\n\n");

	// Write colors (all white)
	fprintf(outFile, "VECTOR colours[%d] = {\n", expanded_count);
	for (int i = 0; i < expanded_count; i++)
	{
		fprintf(outFile, "  { 1.00f, 1.00f, 1.00f, 1.00f }%s\n", (i + 1 == expanded_count) ? "" : ",");
	}
	fprintf(outFile, "};\n");

	fclose(outFile);
	return 1;
}

int main(int argc, char** argv)
{
	if (argv[1] == NULL || argv[1] == "--help" || argv[1] == "-h")
	{
		printf("ObjConversor - Simple OBJ file parser\n");
		printf("Use: ObjConversor <input file> <output file>\n\n");

		printf("On export on blender use these settings:\n");
		printf("  - Include UV Coordinates: ON\n");
		printf("  - Triangulate Mesh: ON\n");
		printf("  - Everything else: OFF\n\n");
		return 1;
	}

	//No exit name
	if (argc < 3)
	{
		perror("Insufficient parameters!\n");
		printf("Use: ObjConversor <input file> <output file>\n");
		return 1;
	}

	FILE* inFile = fopen(argv[1], "r");

	if (inFile == NULL)
	{
		perror("Error opening input file!\n");
		return 1;
	}

	//Read OBJ file per line
	char line[256]; //line size
	while (fgets(line, sizeof(line), inFile))
	{
		if (strncmp(line, "v ", 2) == 0) //Search for vertex 
		{
			Vec3 v;
			sscanf_s(line, "v %f %f %f", &v.x, &v.y, &v.z);
			vertices[vert_count++] = v;
		}
		else if (strncmp(line, "vt ", 3) == 0) //Search for texture coordinate
		{
			Vec2 vt;
			sscanf_s(line + 3, "%f %f", &vt.u, &vt.v);
			texcoords[tex_count++] = vt;
		}
		else if (strncmp(line, "f ", 2) == 0) //Search for face
		{
			Face face;
			sscanf_s(line + 2, "%d/%d %d/%d %d/%d", &face.v[0], &face.vt[0], &face.v[1], &face.vt[1], &face.v[2], &face.vt[2]);

			for (int j = 0; j < 3; j++) { face.v[j]--; face.vt[j]--; }
			faces[face_count++] = face;
		}
	}

	fclose(inFile);

	if (!export_c(argv[2]))
	{
		perror("Error exporting to output file!\n");
		return 1;
	}
	else
	{
		printf("Exported successfully to %s\n", argv[2]);
	}

	return 0;
}
