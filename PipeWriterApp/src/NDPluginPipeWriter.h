#ifndef NDPluginPipeWriter_H
#define NDPluginPipeWriter_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>
#include "NDPluginFile.h"



class epicsShareClass NDPluginPipeWriter : public NDPluginFile {
public:
    NDPluginPipeWriter(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    //void processCallbacks(NDArray *pArray);
    virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray);
    virtual asynStatus readFile(NDArray **pArray);
    virtual asynStatus writeFile(NDArray *pArray);
    virtual asynStatus closeFile();

protected:
    //None

private:
    static const char* pluginName;
    FILE *inFile;
    bool is_pipe_open;
    int lastFrameNumber;
};
#define NUM_PIPE_READER_PARAMS 0

#endif
