// Windows includes (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <math.h>
#include <vector> // STL dynamic memory.

// OpenGL includes
#include <GL/glew.h>
#include <GL/freeglut.h>

// Assimp includes
#include <assimp/cimport.h> // scene importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations

// Project includes
#include "maths_funcs.h"
#include "Camera.h"

unsigned int vp_vbo = 0;
unsigned int vn_vbo = 0;
unsigned int vao = 0;

GLuint VAO[8];

/*----------------------------------------------------------------------------
MESH TO LOAD
----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here
#define MESH_NAME "Models/PlayerShip.dae" //"monkeyhead_smooth.dae", player

#define MESH_Alien1 "Models/AlienShip1.dae"
#define MESH_Alien2 "Models/AlienBase.dae"
#define MESH_NAME2 "Models/ShipTail.dae"
#define MESH_BG "Models/Background.dae" 
#define MESH_test "Models/SceneLayout.dae"

#define MESH_Bullet "Models/Bullet.dae"
#define MESH_cit "Models/Citizen1.dae"

/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

#pragma region SimpleTypes
typedef struct
{
	size_t mPointCount = 0;
	std::vector<vec3> mVertices;
	std::vector<vec3> mNormals;
	std::vector<vec2> mTextureCoords;
} ModelData;
#pragma endregion SimpleTypes

using namespace std;
GLuint shaderProgramID;

ModelData mesh_data;
ModelData mesh_data2;
ModelData mesh_data3;
ModelData mesh_data4;
ModelData mesh_data5;
ModelData mesh_data6;
ModelData mesh_data7;
ModelData test_data;
unsigned int mesh_vao = 0;
int width = 800;
int height = 600;

GLuint loc1, loc2, loc3;
GLfloat rotate_y, rotateX = 0.0f, rotateY = 0.0f, rotateZ = 0.0f;
GLfloat moveX = 0.0f, moveY = 0.0f, moveZ = 0.0f;
GLfloat scaleX = 1, scaleY = 1, scaleZ = 1;

GLfloat camHX = 0.0f, camHY = 0.0f, camHZ = 0.0f; //camera hold values
GLfloat camX = 0.0f, camY = 0.0f, camZ = 0.0f;
GLfloat rotCam = 0.0f;

bool firstCam = false, shot = false;
int swapDir = 0, keepDir = 1, bulletX = 0;
float speed = 0;


#pragma region MESH LOADING
/*----------------------------------------------------------------------------
MESH LOADING FUNCTION
----------------------------------------------------------------------------*/

ModelData load_mesh(const char* file_name) {
	ModelData modelData;

	/* Use assimp to read the model file, forcing it to be read as    */
	/* triangles. The second flag (aiProcess_PreTransformVertices) is */
	/* relevant if there are multiple meshes in the model file that   */
	/* are offset from the origin. This is pre-transform them so      */
	/* they're in the right position.                                 */
	const aiScene* scene = aiImportFile(
		file_name, 
		aiProcess_Triangulate | aiProcess_PreTransformVertices
	); 

	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		return modelData;
	}

	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);

	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		const aiMesh* mesh = scene->mMeshes[m_i];
		printf("    %i vertices in mesh\n", mesh->mNumVertices);
		modelData.mPointCount += mesh->mNumVertices;
		for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
			if (mesh->HasPositions()) {
				const aiVector3D* vp = &(mesh->mVertices[v_i]);
				modelData.mVertices.push_back(vec3(vp->x, vp->y, vp->z));
			}
			if (mesh->HasNormals()) {
				const aiVector3D* vn = &(mesh->mNormals[v_i]);
				modelData.mNormals.push_back(vec3(vn->x, vn->y, vn->z));
			}
			if (mesh->HasTextureCoords(0)) {
				const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
				modelData.mTextureCoords.push_back(vec2(vt->x, vt->y));
			}
			if (mesh->HasTangentsAndBitangents()) {
	
			}
		}
	}

	aiReleaseImport(scene);
	return modelData;
}

#pragma endregion MESH LOADING

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
char* readShaderSource(const char* shaderFile) {
	FILE* fp;
	fopen_s(&fp, shaderFile, "rb");

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);
	buf[size] = '\0';

	fclose(fp);

	return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		std::cerr << "Error creating shader..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	const char* pShaderSource = readShaderSource(pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024] = { '\0' };
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		std::cerr << "Error compiling "
			<< (ShaderType == GL_VERTEX_SHADER ? "vertex" : "fragment")
			<< " shader program: " << InfoLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		std::cerr << "Error creating shader program..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, "simpleVertexShader.txt", GL_VERTEX_SHADER);
	AddShader(shaderProgramID, "simpleFragmentShader.txt", GL_FRAGMENT_SHADER);
	//AddShader(shaderProgramID, "vshader53.glsl", GL_VERTEX_SHADER);
	//AddShader(shaderProgramID, "fshader53.glsl", GL_FRAGMENT_SHADER);


	GLint Success = 0;
	GLchar ErrorLog[1024] = { '\0' };
	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS
void generateObjectBufferMesh(ModelData mesh_n) {
	/*----------------------------------------------------------------------------
	LOAD MESH HERE AND COPY INTO BUFFERS
	----------------------------------------------------------------------------*/

	//Note: you may get an error "vector subscript out of range" if you are using this code for a mesh that doesnt have positions and normals
	//Might be an idea to do a check for that before generating and binding the buffer.

		unsigned int vp_vbo = 0;
		loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
		loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
		loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");

		glGenBuffers(1, &vp_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
		glBufferData(GL_ARRAY_BUFFER, mesh_n.mPointCount * sizeof(vec3), &mesh_n.mVertices[0], GL_STATIC_DRAW);
		unsigned int vn_vbo = 0;
		glGenBuffers(1, &vn_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
		glBufferData(GL_ARRAY_BUFFER, mesh_n.mPointCount * sizeof(vec3), &mesh_n.mNormals[0], GL_STATIC_DRAW);

		//	This is for texture coordinates which you don't currently need, so I have commented it out
		//	unsigned int vt_vbo = 0;
		//	glGenBuffers (1, &vt_vbo);
		//	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
		//	glBufferData (GL_ARRAY_BUFFER, mesh_n.mTextureCoords * sizeof (vec2), &mesh_n.mTextureCoords[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(loc1);
		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
		glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		glEnableVertexAttribArray(loc2);
		glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
		glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL); 

		//This is for texture coordinates which you don't currently need, so I have commented it out
		//glEnableVertexAttribArray (loc3);
		//glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
		//glVertexAttribPointer (loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);

}

#pragma endregion VBO_FUNCTIONS


void display() {

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderProgramID);
	//glEnable(GL_LIGHTING);

	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation(shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation(shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation(shaderProgramID, "proj");

	int light_loc = glGetUniformLocation(shaderProgramID, "light_pos_z");

	// Root of the Hierarchy
	mat4 view = identity_mat4();
	mat4 persp_proj = perspective(45.0f, (float)width / (float)height, 0.1f, 1000.0f);
	mat4 model = identity_mat4();

	model = scale(model, vec3(scaleX, scaleY, scaleZ));
	model = rotate_x_deg(model, rotateX);
	model = rotate_y_deg(model, rotateY);
	view = rotate_y_deg(view, rotCam);
	model = rotate_z_deg(model, rotateZ);
	model = translate(model, vec3(moveX, moveY, moveZ));
	view = translate(view, vec3(camX, camY, camZ - 30.0f));


	//model = translate * rotate * scale;
	// update uniforms & draw
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, model.m);
	glUniform1f(light_loc, -30);
	glBindVertexArray(VAO[0]);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);


	// Set up the child matrix; for Tail
	mat4 modelChild = identity_mat4();
	modelChild = rotate_y_deg(modelChild, 180);
	modelChild = rotate_x_deg(modelChild, rotate_y);
	modelChild = translate(modelChild, vec3(-4.5f, 0.0f, 0.0f));

	// Apply the root matrix to the child matrix
	modelChild = model * modelChild;

	// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, modelChild.m);
	glBindVertexArray(VAO[3]);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data4.mPointCount);

	//glUniformMatrix4fv(ObjectModelView, 1, GL_TRUE, view);
	/*glUniform4fv(ambi, 1, ambient_product);
	glUniform4fv(diff, 1, diffuse_product);
	glUniform4fv(spec, 1, specular_product);
	glUniform1f(shin, material_shininess);*/
	

	//models no movement

	//bg
	mat4 bg = identity_mat4();
	bg = translate(bg, vec3(0.0f, -10.0f, -2.0));
	bg = rotate_y_deg(bg, -90);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, bg.m);
	glBindVertexArray(VAO[4]);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data5.mPointCount);

	//citizens
	mat4 cit1 = identity_mat4();
	cit1 = translate(cit1, vec3(8.0f, -20.0f, 0.0));
	cit1 = scale(cit1, vec3(.5, .5, .5));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, cit1.m);
	glBindVertexArray(VAO[6]);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data7.mPointCount);

	mat4 cit2 = identity_mat4();
	cit2 = translate(cit2, vec3(-45.0f, -20.0f, 0.0));
	cit2 = scale(cit2, vec3(.5, .5, .5));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, cit2.m);
	glBindVertexArray(VAO[6]);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data7.mPointCount);

	mat4 cit3 = identity_mat4();
	cit3 = translate(cit3, vec3(35.0f, -20.0f, 0.0));
	cit3 = scale(cit3, vec3(.5, .5, .5));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, cit3.m);
	glBindVertexArray(VAO[6]);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data7.mPointCount);

	//Alien ship
	mat4 Al1 = identity_mat4();
	Al1 = translate(Al1, vec3(-20.0f, 9.0f, 0.0));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, Al1.m);
	glBindVertexArray(VAO[1]);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data2.mPointCount);
	//Alien Base
	mat4 base1 = identity_mat4();
	base1 = rotate_y_deg(base1, rotate_y);
	base1 = translate(base1, vec3(0.0f, -0.2f, 0.0f));

	base1 = Al1 * base1;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, base1.m);
	glBindVertexArray(VAO[2]);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data3.mPointCount);

	//Alien ship 2
	mat4 Al2 = identity_mat4();
	Al2 = translate(Al2, vec3(30.0f, 9.0f, 0.0));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, Al2.m);
	glBindVertexArray(VAO[1]);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data2.mPointCount);
	//Alien Base
	mat4 base2 = identity_mat4();
	base2 = rotate_y_deg(base2, rotate_y);
	base2 = translate(base2, vec3(0.0f, -0.2f, 0.0f));

	base2 = Al2 * base2;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, base2.m);
	glBindVertexArray(VAO[2]);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data3.mPointCount);

	//bullets
	mat4 bul1 = identity_mat4();
	bul1 = translate(bul1, vec3(bulletX, -0.3f, 0.0f));
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, bul1.m);
	glBindVertexArray(VAO[5]);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data6.mPointCount);

	glutSwapBuffers();
}


void updateScene() {

	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	// Rotate the model slowly around the y axis at 20 degrees per second
	rotate_y += 20.0f * delta;
	rotate_y = fmodf(rotate_y, 360.0f);

	speed += 20.0f * delta;

	bulletX = moveX;

	if (shot == true) {
		if (keepDir == 0)
			bulletX = -speed;
		else
			bulletX = speed;
	}

	glutPostRedisplay();
}


void init()
{
	GLuint shaderProgramID = CompileShaders();

	// load mesh into a vertex buffer array
	glGenVertexArrays(7, VAO);
	glBindVertexArray(VAO[0]);
	mesh_data = load_mesh(MESH_NAME);
	generateObjectBufferMesh(mesh_data);

	glBindVertexArray(VAO[1]);
	mesh_data2 = load_mesh(MESH_Alien1);
	generateObjectBufferMesh(mesh_data2);

	glBindVertexArray(VAO[2]);
	mesh_data3 = load_mesh(MESH_Alien2);
	generateObjectBufferMesh(mesh_data3);

	glBindVertexArray(VAO[3]);
	mesh_data4 = load_mesh(MESH_NAME2);
	generateObjectBufferMesh(mesh_data4);

	glBindVertexArray(VAO[4]);
	mesh_data5 = load_mesh(MESH_BG);
	generateObjectBufferMesh(mesh_data5);

	glBindVertexArray(VAO[5]);
	mesh_data6 = load_mesh(MESH_Bullet);
	generateObjectBufferMesh(mesh_data6);

	glBindVertexArray(VAO[6]);
	mesh_data7 = load_mesh(MESH_cit);
	generateObjectBufferMesh(mesh_data7);

	//test_data = load_mesh(MESH_test);

}

void keypress(unsigned char key, int x, int y) {

	int number = -1;
	//move_camera(number, key);

	switch (key) {
	case 033: //escape key
	case 'q': case 'Q':
		exit(EXIT_SUCCESS);
		break;
	case 'c': //swapCam
		if (firstCam == false) {
			firstCam = true;
			rotCam = 90;
			camX = 0;
			camY = 2;
			camZ = 0;
		}
		else if (firstCam == true) {
			firstCam = false;
			rotCam = 0;

			camX = -1 * moveX;
			camY = 0;
			camZ = 0;
		}
		//scaleX = 1.0f;
		break;
	case ' ':  // reset values to their defaults
		std::cerr << "Change to shoot button" << std::endl;

		shot = true;

		break;
	//scale uniform (using - +)
	/*case '-':
		scaleX -= 0.1f;
		scaleY -= 0.1f;
		scaleZ -= 0.1f;
		break;
	case '+':
		scaleX += 0.1f;
		scaleY += 0.1f;
		scaleZ += 0.1f;
		break;
	//scale Separately (using IOP)
	case 'i':
		scaleX -= 0.1f;
		break;
	case 'o':
		scaleY -= 0.1f;
		break;
	case 'p':
		scaleZ -= 0.1f;
		break;
	case 'I': //capitals = +
		scaleX += 0.1f;
		break;
	case 'O':
		scaleY += 0.1f;
		break;
	case 'P':
		scaleZ += 0.1f;
		break;
	//Rotate (using ERT)
	case 'e':
		rotateX -= 0.5f;
		break;
	case 'r':
		rotateY -= 0.5f;
		break;
	case 't':
		rotateZ -= 0.5f;
		break;
	case 'E': //capitals = +
		rotateX += 0.5f;
		break;
	case 'R':
		rotateY += 0.5f;
		break;
	case 'T':
		rotateZ += 0.5f;
		break;*/
	//Tanslate (WASD keys plus x and z for Z axis)
	case 'w':
			moveY += 0.1f;
			if (moveY > 10)
				moveY = 10;
		else if (firstCam == true) {
			moveY += 0.1f;
		}
		break;
	case 'a':
		keepDir = 0; //facing left
		scaleX = -1.0f;
		moveX -= 0.1f;

		if (firstCam == false)
			camX = -1 * moveX; //camX += 0.1f;

		if (moveX < -25) {
			moveX = -25;
			//camX = 25;
			//std::cerr << "CamX: " << camX << std::endl;
		}
		else if (firstCam == true) {
			camZ -= 0.1f;
		}
		break;
	case 's':
		moveY -= 0.1f;
		//std::cerr << "x: " << moveX << " y: " << moveY << std::endl; //4.1 -1.1
		if (moveY < -5.6)
			moveY = -5.6;
		break;
	case 'd':
		keepDir = 1; //facing right
		moveX += 0.1f;
		scaleX = 1.0f;

		if (firstCam == false)
			camX = -1 * moveX; // camX -= 0.1f;

		if (moveX > 25) {
			moveX = 25;
		}
		else if (firstCam == true) {
			camZ += 0.1f;
		}
		break;
	/*case 'z':
		moveZ -= 0.1f;   //translate Z
		break;
	case 'x':
		moveZ += 0.1f;
		std::cerr << moveX << moveZ << std::endl;
		break; */	
	}
}

int main(int argc, char** argv) {

	std::cerr << "WASD keys to move." << std::endl;
	std::cerr << "Space bar to shoot." << std::endl;
	std::cerr << "Change camera with C key." << std::endl;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("Defender Game");

	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);

	GLenum res = glewInit();
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}

	init();
	
	glutMainLoop();

	return 0;
}
