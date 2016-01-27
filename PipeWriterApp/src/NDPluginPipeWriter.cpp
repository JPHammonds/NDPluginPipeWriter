
#include <epicsMutex.h>
#include <iocsh.h>
#include <asynDriver.h>
#include <epicsExport.h>

#include "pipebinaryformat.h"
#include "NDPluginPipeWriter.h"

/* define C99 standard __func__ to come from MS's __FUNCTION__ */
#if defined ( _MSC_VER )
#define __func__ __FUNCTION__
#endif

const char* NDPluginPipeWriter::pluginName = "NDPluginPipeWriter";

asynStatus NDPluginPipeWriter::openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray)
{
   int status = asynSuccess;

   if (!is_pipe_open) {
       inFile = fopen(fileName, "wb");
 
       if (inFile != 0) {
           asynPrint( pasynUserSelf, ASYN_TRACE_FLOW,
                      "%s:%s open pipe %s\n",
                      pluginName,
                      __func__,
                      fileName);
           is_pipe_open = true;
           status = asynSuccess;
       }
       else {
           asynPrint( pasynUserSelf, ASYN_TRACE_ERROR,
                     "%s:%s FAILED to open pipe %s\n",
                     pluginName,
                     __func__,
                     fileName);
            is_pipe_open = false;
            status = asynError;
        }
   }
   return (asynStatus)status;
}


asynStatus NDPluginPipeWriter::writeFile(NDArray *pArray)
{
    int status = asynSuccess;
    pipeBinaryFormat pFormat;
    int sizeX = (int)pArray->dims[0].size;
    int sizeY = (int)pArray->dims[1].size;
    int sizePixels = sizeX * sizeY;
    unsigned short *pData = (unsigned short*)pArray->pData;
    //Warn if we are seeing identical frame numbers
    if ( pArray->uniqueId == lastFrameNumber ) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
	          "%s:%s pArray->uniqueId == lastFrameNumber %d\n",
		  pluginName, __FUNCTION__,
                  pArray->uniqueId);
    }
    //Warn if frame numbers are not sequential
    if ( lastFrameNumber != -1 && (pArray->uniqueId - lastFrameNumber) != 1) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s:%s %d frames missing starting at %d",
		  pluginName, __FUNCTION__,
		  pArray->uniqueId - lastFrameNumber,
		  pArray->uniqueId + 1);
    }
    lastFrameNumber = pArray->uniqueId;
    int frameNumber = pArray->uniqueId;
    int inputImageCount = pArray->uniqueId;
    int errorCode = 0;
    
    
    pFormat.writeDataBlock(inFile,
            pData,
            sizeX,
            sizeY,
            sizePixels,
            frameNumber,
            inputImageCount,
            errorCode);

    return (asynStatus)status;
}

/** Reads single NDArray from a Null file; NOT CURRENTLY IMPLEMENTED.
  * \param[in] pArray Pointer to the NDArray to be read
  */
asynStatus NDPluginPipeWriter::readFile(NDArray **pArray)
{
   int status = asynSuccess;

   // Not currently implemented, return asynError
   status = asynError;
   return (asynStatus)status;
}

/** Closes the Null file. */
asynStatus NDPluginPipeWriter::closeFile()
{
    int status = asynSuccess;
    int fstat;
    if (is_pipe_open) {
        fstat = fclose(inFile);
        status = asynSuccess;
        if (fstat == EOF) {
            asynPrint( pasynUserSelf, ASYN_TRACE_ERROR,
                       "%s:%s Trouble closing pipe\n",
                       pluginName,
                       __func__);
            status = asynError;
        }
        is_pipe_open = false;
    }
    return (asynStatus)status;
}

NDPluginPipeWriter::NDPluginPipeWriter(const char *portName, int queueSize, int blockingCallbacks,
             const char *NDArrayPort, int NDArrayAddr, int maxBuffers, size_t maxMemory,
             int priority, int stackSize)
  /* Invoke the base class constructor */
  : NDPluginFile(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_PIPE_READER_PARAMS, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   ASYN_CANBLOCK, 1, priority, stackSize)
{
    is_pipe_open = false;
    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginPipeWriter");
    lastFrameNumber = -1;
    this->supportsMultipleArrays = 1;

}


/** Configuration command */
extern "C" int NDPipeWriterConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                    const char *NDArrayPort, int NDArrayAddr,
                                    int maxBuffers, size_t maxMemory,
                                    int priority, int stackSize)
{
  new NDPluginPipeWriter(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
              maxBuffers, maxMemory, priority, stackSize);
  return(asynSuccess);
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg6 = { "maxMemory",iocshArgInt};
static const iocshArg initArg7 = { "priority",iocshArgInt};
static const iocshArg initArg8 = { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8};
static const iocshFuncDef initFuncDef = {"NDPipeWriterConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
  NDPipeWriterConfigure(args[0].sval, args[1].ival, args[2].ival,
                       args[3].sval, args[4].ival, args[5].ival,
                       args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void NDPipeWriterRegister(void)
{
  iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
  epicsExportRegistrar(NDPipeWriterRegister);
}
