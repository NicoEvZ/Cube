#include <math.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include "draw.h"
#include "cJSON.h"

typedef struct data
{
    double distance;
    double scale;
    char objPathBuffer[64];
    int i;
    int rotationX;
    int rotationY;
    int rotationZ;
    int screenWidthImprt;
    int screenHeightImprt;
}data;

int importJSON(const char *file_path, data *importData_struct)
{
    // Open the file for reading
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    // Determine the size of the file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read the content of the file into a buffer
    char *json_buffer = (char *)malloc(file_size + 1);
    fread(json_buffer, 1, file_size, file);
    fclose(file);

    // Null-terminate the buffer
    json_buffer[file_size] = '\0';

    // Parse the JSON from the buffer
    cJSON *root = cJSON_Parse(json_buffer);

    if (root == NULL) {
        printf("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        free(json_buffer);
        return 1;
    }

    // Access values in the JSON object    
    cJSON *distanceJSON = cJSON_GetObjectItemCaseSensitive(root, "distance");
    cJSON *scaleJSON = cJSON_GetObjectItemCaseSensitive(root, "scale");
    cJSON *objFileJOSN = cJSON_GetObjectItemCaseSensitive(root, "objFile");
    cJSON *iterationsJSON = cJSON_GetObjectItemCaseSensitive(root, "iterations");
    cJSON *rotateXJSON = cJSON_GetObjectItemCaseSensitive(root, "rotateX");
    cJSON *rotateYJSON = cJSON_GetObjectItemCaseSensitive(root, "rotateY");
    cJSON *rotateZJSON = cJSON_GetObjectItemCaseSensitive(root, "rotateZ");
    cJSON *screenWidthJSON = cJSON_GetObjectItemCaseSensitive(root, "screenWidth");
    cJSON *screenHeightJSON = cJSON_GetObjectItemCaseSensitive(root, "screenHeight");


    if(cJSON_IsNumber(distanceJSON))
    {
        importData_struct->distance = distanceJSON->valuedouble;
    }

    if(cJSON_IsNumber(scaleJSON))
    {
        importData_struct->scale = scaleJSON->valuedouble;
    }

    if(cJSON_IsNumber(iterationsJSON))
    {
        importData_struct->i = iterationsJSON->valueint;
    }

    if(cJSON_IsBool(rotateXJSON) & cJSON_IsBool(rotateYJSON) & cJSON_IsBool(rotateZJSON))
    {
        importData_struct->rotationX = rotateXJSON->valueint;
        importData_struct->rotationY = rotateYJSON->valueint;
        importData_struct->rotationZ = rotateZJSON->valueint;
    }

    if(cJSON_IsString(objFileJOSN))
    {
        strncpy(importData_struct->objPathBuffer,objFileJOSN->valuestring,sizeof(importData_struct->objPathBuffer) -1);
        importData_struct->objPathBuffer[sizeof(importData_struct->objPathBuffer) -1] = '\0';
    }

    if(cJSON_IsNumber(screenWidthJSON) & cJSON_IsNumber(screenHeightJSON))
    {
        importData_struct->screenWidthImprt = screenWidthJSON->valueint;
        importData_struct->screenHeightImprt = screenHeightJSON->valueint;
    }

    // Don't forget to free the cJSON object and the buffer when you're done with them
    cJSON_Delete(root);
    free(json_buffer);

    return 0;
}

void initScreen(screenStruct *screen)
{
    screen->screen = malloc(screen->width * sizeof(int *));
    for(int i = 0; i < screen->width; i++)
    {
        screen->screen[i] = malloc(screen->height * sizeof(int));
    }
}

int main(void){
    data importData;
    screenStruct screen;

    const char jsonImportPath[] = "data/inputData.json";

    if (importJSON(jsonImportPath, &importData))
    {
        printf("Import of JSON failed, exiting here");
        return 1;
    }

    screen.width = importData.screenWidthImprt;
    screen.height = importData.screenHeightImprt;

    initScreen(&screen);

    const double half_x = screen.width/2;
    const double half_y = screen.height/2;
    double ratio = screen.width/screen.height;
    double angle = 0;

    //screenspace center, not 3d space center
    double origin[]={half_x,half_y}; //origin is middle of screenspace

    //Store OBJ data in mesh struct
    mesh baseMesh = importMeshFromOBJFile(importData.objPathBuffer); 
    mesh rotatedMesh;
    mesh projectedMesh;
    
    //failsafe that exits code if the importMeshFromOBJFile didn't succeed.
    if (baseMesh.numOfTris == 0)
    {
        return 0;
    }
 
    //display 
    for (int i = 0; i < importData.i; i++)
    {
        //clear screen
        clearScreen(&screen);

        rotatedMesh = copyMeshData(baseMesh, rotatedMesh);

        // rotate around axes
        if(importData.rotationX)
        {
            rotatedMesh = rotateMeshAroundX(rotatedMesh, (angle * (PI/180)));
        }

        if(importData.rotationY)
        {
            rotatedMesh = rotateMeshAroundY(rotatedMesh, (angle * (PI/180)));
        }

        if(importData.rotationZ)
        {
            rotatedMesh = rotateMeshAroundZ(rotatedMesh, (angle * (PI/180)));
        }
        projectedMesh = copyMeshData(rotatedMesh, projectedMesh);

        // distance =  30 * (sin(0.1 * i)+ 1.8);

        //project 3D --> 2D
        projectMeshTo2D(projectedMesh, importData.distance);
        
        //scale points
        scale2DPoints(projectedMesh,importData.scale);

        //draw lines
        drawMeshOnScreen(projectedMesh, origin, ratio, screen);

        displayScreen(&screen);

        angle = angle + 1;
        nanosleep((const struct timespec[]){{0, 83300000L}}, NULL);
    }
    return 0;
}