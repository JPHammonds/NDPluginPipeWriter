
#include <cstdio>
//#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <cstdlib>

#include <epicsMutex.h>
#include <iocsh.h>
#include <asynDriver.h>
#include <epicsExport.h>
#include <epicsThread.h>
#include <epicsExit.h>

#include "pipebinaryformat.h"
#include "NDPluginPipeWriter.h"

using std::string;
using std::endl;
using std::vector;

/* define C99 standard __func__ to come from MS's __FUNCTION__ */
#if defined ( _MSC_VER )
#define __FUNCTION__ __func__
#endif

#if defined(_WIN32)              // Windows
  #include <direct.h>
  #define strtok_r(a,b,c) strtok(a,b)
  #define MKDIR(a,b) _mkdir(a)
  #define delim "\\"
#elif defined(vxWorks)           // VxWorks
  #include <sys/stat.h>
  #define MKDIR(a,b) mkdir(a)
  #define delim "/"
#else                            // Linux
  #include <sys/stat.h>
  #include <sys/types.h>
  #define delim "/"
  #define MKDIR(a,b) mkdir(a,b)
#endif

extern "C" {

/**
 * Callback function for exit hook
 */
static void exitCallbackC(void *pPvt) {
    NDPluginPipeWriter *pPipeWriter = (NDPluginPipeWriter*) pPvt;
    delete pPipeWriter;
}
/**
 * Callback when a new Image event is seen.  Call the driver's method
 * piHanfleNewImageTask
 */
static void watchMPICommandOutputTaskC(void *drvPvt)
{
    NDPluginPipeWriter *pPvt = (NDPluginPipeWriter *)drvPvt;

    pPvt->watchMPICommandOutputTask();
}

}

const char* NDPluginPipeWriter::pluginName = "NDPluginPipeWriter";
const char* NDPluginPipeWriter::mpiProgName = "xpcsMPI";
string NDPluginPipeWriter::intStr = "int";
string NDPluginPipeWriter::boolStr = "bool";
string NDPluginPipeWriter::qstringStr = "QString";
string NDPluginPipeWriter::windowStr = "window";
string NDPluginPipeWriter::xSizeCmd = "on_spinBox_imgSizeX_valueChanged";
string NDPluginPipeWriter::ySizeCmd = "on_spinBox_imgSizeY_valueChanged";
string NDPluginPipeWriter::dataSourceTestImagesCmd = "on_radioButton_testimages_clicked";
string NDPluginPipeWriter::dataSourceLinuxPipeCmd = "on_radioButton_linuxpipe_clicked";
string NDPluginPipeWriter::numTestImagesCmd = "on_spinBox_numImgs_valueChanged";
string NDPluginPipeWriter::testImagePeriodCmd = "on_spinBox_imgPeriodMs_valueChanged";
string NDPluginPipeWriter::infiniteInImagesCmd = "on_checkBox_infiniteInImgs_clicked";
string NDPluginPipeWriter::descrambleSelectCmd = "on_checkBox_descramble_clicked";
string NDPluginPipeWriter::nToAvgCmd = "on_lineEdit_ntoavg_textChanged";
string NDPluginPipeWriter::startClickedCmd = "on_pushButton_start_clicked";
string NDPluginPipeWriter::stopClickedCmd = "on_pushButton_stop_clicked";
string NDPluginPipeWriter::acqDarksClickedCmd = "on_pushButton_scqdarks_clicked";
string NDPluginPipeWriter::subDarksClickedCmd = "on_checkBox_subdark_clicked";
string NDPluginPipeWriter::inQueueLenChangedCmd = "on_spinBox_inQueueLen_valueChanged";
string NDPluginPipeWriter::outQueueLenChangedCmd = "on_spinBox_outQueueLen_valueChanged";
string NDPluginPipeWriter::resetQueuesClickedCmd = "on_pushButton_resetInQueue_clicked";
string NDPluginPipeWriter::tiffNumChangedCmd = "on_linEdit_tiffnumber_textChanged";
string NDPluginPipeWriter::nullOutputSelectedCmd = "on_radioButton_nullOutput_clicked";
string NDPluginPipeWriter::tiffOutputSelectedCmd = "on_radioButton_tifffile_clicked";
string NDPluginPipeWriter::immOutputSelectedCmd = "on_radioButton_immFileOutput_clicked";
string NDPluginPipeWriter::linuxPipeOutSelectedCmd = "on_radioButton_linuxOutPipe_clicked";
string NDPluginPipeWriter::rawImgOutSelectedCmd = "on_radioButton_noImm_clicked";
string NDPluginPipeWriter::rawImmOutSelectedCmd= "on_radioButton_rawIMM_clicked";
string NDPluginPipeWriter::compImmOutSelectedCmd = "on_radioButton_compIMM_clicked";
string NDPluginPipeWriter::num2CaptureChangedCmd = "on_lineEdit_NumFiles2Capture_textEdited";
string NDPluginPipeWriter::outFilePathChangedCmd = "on_lineEdit_tiffpath_textChanged";
string NDPluginPipeWriter::outFileBaseNameChangedCmd = "on_lineEdit_tiffbasename_textChanged";
string NDPluginPipeWriter::outIncFileNumCmd = "on_checkBox_IncFileNum_clicked";
string NDPluginPipeWriter::outNextNumberCmd = "on_linEdit_tiffnumber_textChanged";
string NDPluginPipeWriter::emptyStr = "";

enum outputType {RAW_IMG, RAW_IMM, COMP_IMM};
enum outputFileType {NULL_OUTPUT, TIFF_FILE, IMM_FILE, LINUX_PIPE_OUT};
enum dataSourceType{TEST_IMAGE, LINUX_PIPE_SOURCE};

/** Checks whether the directory specified NDFilePath parameter exists.
  *
  * This is a convenience function that determinesthe directory specified NDFilePath parameter exists.
  * It sets the value of NDFilePathExists to 0 (does not exist) or 1 (exists).
  * It also adds a trailing '/' character to the path if one is not present.
  * Returns a error status if the directory does not exist.
  */
asynStatus NDPluginPipeWriter::checkOutputPath()
{
    /* Formats a complete file name from the components defined in NDStdDriverParams */
    asynStatus status = asynError;
    char filePath[MAX_FILENAME_LEN];
    int hasTerminator=0;
    struct stat buff;
    int istat;
    size_t len;
    int isDir=0;
    int pathExists=0;

    getStringParam(PipeWriter_OutputFilePath, sizeof(filePath), filePath);
    len = strlen(filePath);
    if (len == 0) return(asynSuccess);
    /* If the path contains a trailing '/' or '\' remove it, because Windows won't find
     * the directory if it has that trailing character */
    if (strncmp(&filePath[len-1], delim, 1) == 0) {
        filePath[len-1] = 0;
        len--;
        hasTerminator=1;
    }
    istat = stat(filePath, &buff);
    if (!istat) isDir = (S_IFDIR & buff.st_mode);
    if (!istat && isDir) {
        pathExists = 1;
        status = asynSuccess;
    }
    /* If the path did not have a trailing terminator then add it if there is room */
    if (!hasTerminator) {
        if (len < MAX_FILENAME_LEN-2) strcat(filePath, delim);
        setStringParam(PipeWriter_OutputFilePath, filePath);
    }
    setIntegerParam(PipeWriter_OutputFilePathExists, pathExists);
    return status;
}



asynStatus NDPluginPipeWriter::openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray)
{
   int status = asynSuccess;

   if (!isPipeOpen) {
       inFile = fopen(fileName, "wb");
 
       if (inFile != 0) {
           asynPrint( pasynUserSelf, ASYN_TRACE_FLOW,
                      "%s:%s open pipe %s\n",
                      pluginName,
                      __func__,
                      fileName);
           isPipeOpen = true;
           status = asynSuccess;
       }
       else {
           asynPrint( pasynUserSelf, ASYN_TRACE_ERROR,
                     "%s:%s FAILED to open pipe %s\n",
                     pluginName,
                     __func__,
                     fileName);
            isPipeOpen = false;
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
    if (isPipeOpen) {
        fstat = fclose(inFile);
        //sendCommand(mpiProgName, windowStr, nullOutputSelectedCmd, 0, emptyStr, emptyStr);
        status = asynSuccess;
        if (fstat == EOF) {
            asynPrint( pasynUserSelf, ASYN_TRACE_ERROR,
                       "%s:%s Trouble closing pipe\n",
                       pluginName,
                       __func__);
            status = asynError;
        }
        isPipeOpen = false;
    }
    return (asynStatus)status;
}

bool NDPluginPipeWriter::closeCommandInputPipe() {
    char inPipeName[256];
    getStringParam(PipeWriter_CommandPipeIn, 256, inPipeName);
    bool rval = false;
    if (cmdInPipe){
        int status;
        status = fclose(cmdInPipe);

        if (status == 0){
            cmdInPipeOpen = false;
            rval = true;
        }
        else {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s%s, Trouble closing command in pipe file\n",
                    pluginName, __FUNCTION__);
            cmdInPipeOpen = false;
        }
    }
    return rval;
}

bool NDPluginPipeWriter::closeCommandOutputPipe() {
    char outPipeName[256];
    getStringParam(PipeWriter_CommandPipeOut, 256, outPipeName);
    bool rval = false;
    if (cmdOutPipe){
        int status;
        status = fclose(cmdOutPipe);

        if (status == 0){
            cmdOutPipeOpen = false;
            rval = true;
        }
        else {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s%s, Trouble closing command out pipe file\n",
                    pluginName, __FUNCTION__);
            cmdOutPipeOpen = false;
        }
    }
    return rval;
}

bool NDPluginPipeWriter::openCommandInputPipe() {
    char inPipeName[256];
    getStringParam(PipeWriter_CommandPipeIn, 256, inPipeName);

    if (!cmdInPipeOpen){
        mkfifo(cmdPipeInName, S_IRUSR| S_IWUSR);

        cmdInPipe = fopen(inPipeName, "r");

        if (cmdInPipe != 0) {
            cmdInPipeOpen = true;
        }
        else {
            cmdInPipeOpen = false;
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s, Problem opening command in pipe %s\n",
                    pluginName, __FUNCTION__,
                    inPipeName);
        }

    }
    return cmdInPipeOpen;
}

bool NDPluginPipeWriter::openCommandOutputPipe() {
    char outPipeName[256];
    getStringParam(PipeWriter_CommandPipeOut, 256, outPipeName);
    if (!cmdOutPipeOpen){
        mkfifo(outPipeName, S_IRUSR| S_IWUSR);

        cmdOutPipe = fopen(outPipeName, "w");

        if (cmdOutPipe != 0) {
            cmdOutPipeOpen = true;
        }
        else {
            cmdOutPipeOpen = false;
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s, Problem opening command out pipe %s\n",
                    pluginName, __FUNCTION__,
                    outPipeName
                    );
        }
    }
    return cmdOutPipeOpen;
}

void NDPluginPipeWriter::sendCommand(const string progName, const string slotName, const string argName, int numArgs, const string argType, string  args){
    std::stringstream cmd;

    cmd << progName << " " << slotName << " " << argName << " ";
    cmd << numArgs;
    cmd << " " << argType << " " << args;
    cmd << endl;

    printf("cmd: %s", cmd.str().c_str());
    fflush(stdout);
    fputs( cmd.str().c_str(), cmdOutPipe);
    fflush(cmdOutPipe);
}

void NDPluginPipeWriter::watchMPICommandOutputTask() {
    char outChars[256];
    char *argToken;
    string token;
    string thisApp("IOC");
    string setParamStr("setParameter");

    while (1 && keepGoing) {
        vector<string> args;
        epicsThreadSleep(0.005);

        fgets(outChars, 255, cmdInPipe);
        argToken = strtok(outChars, " ");
        while (argToken != NULL) {
            args.push_back(argToken);
            argToken = strtok(NULL, " ");
        }
        if ( args.size() >= 4){
            string app=args.at(0);
            string functionName = args.at(1);
            string paramName = args.at(2);
            int argCount = std::atoi(args.at(3).c_str());
            if ((app.compare(thisApp) == 0) && (functionName.compare(setParamStr) == 0 )) {
                if (argCount==0) {

                }
                else if (argCount == 1) {
                    string argtype = args.at(4);
                    string argval = args.at(5);

                    if (argtype == "int"){
                        int status = asynError;
                        int paramRef;
                        int iVal = std::atoi(argval.c_str());
                        asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                                "%s%s. Writing int parameter %s value %d\n",
                                pluginName, __FUNCTION__,
                                paramName.c_str(), iVal);
                        status = findParam(paramName.c_str(), &paramRef);
                        if (status == asynSuccess) {
                            int tempVal;
                            getIntegerParam(paramRef, &tempVal);
                            if (tempVal != iVal) {
                                status = setIntegerParam(paramRef, iVal);
                            }
                        }
                        else {
                            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                                    "%s%s. Trouble finding int parameter %s\n",
                                    pluginName, __FUNCTION__,
                                    paramName.c_str());
                        }
                    }
                    else if (argtype == "double"){
                        int status = asynError;
                        int paramRef;
                        double dVal = std::atof(argval.c_str());
                        asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                                "%s, %s, Writing floatt parameter %s value %f\n",
                                pluginName, __FUNCTION__,
                                paramName.c_str(), dVal);
                        status = findParam(paramName.c_str(), &paramRef);
                        if (status == asynSuccess) {
                            double tempVal;
                            getDoubleParam(paramRef, &tempVal);
                            if (tempVal != dVal) {
                                status = setDoubleParam(paramRef, dVal);
                            }
                        }
                        else {
                            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                                    "%s%s. Trouble finding double parameter %s\n",
                                    pluginName, __FUNCTION__,
                                    paramName.c_str());
                        }
                    }
                    else if (argtype == "QString"){
                        int status = asynError;
                        int paramRef;
                        asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                                "%s%s Writing string parameter %s value %s\n",
                                pluginName, __FUNCTION__,
                                paramName.c_str(), argval.c_str());

                        status = findParam(paramName.c_str(), &paramRef);
                        if (status == asynSuccess) {
                            char tempVal[256];
                            getStringParam(paramRef, 256, tempVal);
                            if (strcmp(tempVal, argval.c_str()) != 0){
                                status = setStringParam(paramRef, (argval.c_str()));
                            }
                        }
                        else {
                            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                                    "%s%s. Trouble finding string parameter %s\n",
                                    pluginName, __FUNCTION__,
                                    paramName.c_str());
                        }
                    }
                    else {
                        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                                "%s:%s  Find string parameter %s value %s\n",
                                pluginName, __FUNCTION__,
                                paramName.c_str(), argval.c_str());

                    }
                }
                else {
                    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                            "%s:%s, Not prepared for this: %s\n",
                            pluginName, __FUNCTION__,
                            outChars);
                }
                callParamCallbacks();
            }
        }
//        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
//                "%s:%s cmdOut %s\n",
//                pluginName, __FUNCTION__,
//                outChars);
    }
}

asynStatus NDPluginPipeWriter::writeInt32(asynUser *pasynUser, epicsInt32 value){
    int status = asynSuccess;
    int function = pasynUser->reason;

    /** Make sure that we write the value to the param
     *
     */
    setIntegerParam(function, value);
    std::stringstream sval;
    sval << value;
    if (function == PipeWriter_XSize) {
        sendCommand(mpiProgName, windowStr,xSizeCmd, 1, intStr, sval.str());
        status = NDPluginFile::writeInt32(pasynUser, value);
    }
    else if (function == PipeWriter_YSize) {
        sendCommand(mpiProgName, windowStr,ySizeCmd, 1, intStr, sval.str());
        status = NDPluginFile::writeInt32(pasynUser, value);
    }
    else if (function == NDFileCapture) {
        std::stringstream captureSval;
        if (value == 0){
            captureSval << NULL_OUTPUT;
            sendCommand(mpiProgName, windowStr, nullOutputSelectedCmd, 0, emptyStr, emptyStr);
        }
        else {
            captureSval << IMM_FILE;
            sendCommand(mpiProgName, windowStr, immOutputSelectedCmd, 1, boolStr, captureSval.str());
        }
        status = NDPluginFile::writeInt32(pasynUser, value);
    }
    else if (function == NDFileNumCapture) {
        sendCommand(mpiProgName, windowStr, num2CaptureChangedCmd, 1, qstringStr, sval.str());
        status = NDPluginFile::writeInt32(pasynUser, value);
    }
    else if (function == PipeWriter_DataSourceType) {
        switch (value){
        case TEST_IMAGE:
            sendCommand(mpiProgName, windowStr, dataSourceTestImagesCmd, 1, boolStr, sval.str());
            break;
        case  LINUX_PIPE_SOURCE:
            sendCommand(mpiProgName, windowStr, dataSourceLinuxPipeCmd, 1, boolStr, sval.str());
            break;
        }
    }
    else if (function == PipeWriter_NumTestImages) {
        sendCommand(mpiProgName, windowStr, numTestImagesCmd, 1, intStr, sval.str());
    }
    else if (function == PipeWriter_TestImagePeriod) {
        sendCommand(mpiProgName, windowStr, testImagePeriodCmd, 1, intStr, sval.str());
    }
    else if (function == PipeWriter_InputInfImages) {
        sendCommand(mpiProgName, windowStr, infiniteInImagesCmd, 1, boolStr, sval.str());
    }
    else if (function == PipeWriter_InputQueueSize) {
        sendCommand(mpiProgName, windowStr, inQueueLenChangedCmd, 1, intStr, sval.str());
    }
    else if (function == PipeWriter_OutputQueueSize) {
        sendCommand(mpiProgName, windowStr, outQueueLenChangedCmd, 1, intStr, sval.str());
    }
    else if (function == PipeWriter_ResetQueues) {
        sendCommand(mpiProgName, windowStr, resetQueuesClickedCmd, 0, emptyStr, emptyStr);
    }
    else if (function == PipeWriter_CalcOutType) {
        switch (value) {
        case RAW_IMG:
            sendCommand(mpiProgName, windowStr, rawImgOutSelectedCmd, 0, emptyStr, emptyStr);
            break;
        case RAW_IMM:
            sendCommand(mpiProgName, windowStr, rawImmOutSelectedCmd, 0, emptyStr, emptyStr);
            break;
        case COMP_IMM:
            sendCommand(mpiProgName, windowStr, compImmOutSelectedCmd, 0, emptyStr, emptyStr);
            break;
        }
    }
    else if (function == PipeWriter_OutputFileType) {
        switch (value) {
        case NULL_OUTPUT:
            sendCommand(mpiProgName, windowStr, nullOutputSelectedCmd, 0, emptyStr, emptyStr);
            break;
        case TIFF_FILE:
            sendCommand(mpiProgName, windowStr, tiffOutputSelectedCmd, 1, boolStr, sval.str());
            break;
        case IMM_FILE:
            sendCommand(mpiProgName, windowStr, immOutputSelectedCmd, 1, boolStr, sval.str());
            break;
        case LINUX_PIPE_OUT:
            sendCommand(mpiProgName, windowStr, linuxPipeOutSelectedCmd, 1, boolStr, sval.str());
            break;
        }
    }
    if (function == PipeWriter_OutputNCapture) {
        //Handled by asynNDArrayDriver's NUM_CAPTURE above
    }
    if (function == PipeWriter_OutputNextNumber) {
        sendCommand(mpiProgName, windowStr, outNextNumberCmd, 1, qstringStr, sval.str());
    }
    if (function == PipeWriter_OutputIncFileNum) {
        sendCommand(mpiProgName, windowStr, outIncFileNumCmd, 1, boolStr, sval.str());
    }
    if (function == PipeWriter_ProcessStart) {
        sendCommand(mpiProgName, windowStr, startClickedCmd, 0, emptyStr, emptyStr);
    }
    if (function == PipeWriter_ProcessStop) {
        sendCommand(mpiProgName, windowStr, stopClickedCmd, 0, emptyStr, emptyStr);
    }
    else if (function < PipeWriter_FIRST_PARAM) {
        status = NDPluginFile::writeInt32(pasynUser, value);
    }


    return (asynStatus)status;
}


asynStatus NDPluginPipeWriter::writeOctet(asynUser *pasynUser, const char *value,
        size_t nChars, size_t *nActual){
    int status = asynSuccess;
    int function = pasynUser->reason;

    char previousValue[256];

    getStringParam(function, 256, previousValue);
    if (strcmp(previousValue, value) != 0){
        setStringParam(function, value);
        strcpy(previousValue, value);
    }
    else {
        return asynSuccess;
    }

    if (function == PipeWriter_CommandPipeIn) {

    }
    else if (function == PipeWriter_CommandPipeOut) {

    }
    else if (function == PipeWriter_OutputFilePath) {
        status = checkOutputPath();
        if (status == asynError) {
            // If the directory does not exist then try to create it
            int pathDepth;
            getIntegerParam(NDFileCreateDir, &pathDepth);
            status = createFilePath(value, pathDepth);
            status = this->checkOutputPath();
        }
        sendCommand(mpiProgName, windowStr, outFilePathChangedCmd, 1, qstringStr, value);
    }
    else if (function == PipeWriter_OutputFileName) {
        sendCommand(mpiProgName, windowStr, outFileBaseNameChangedCmd, 1, qstringStr, value);
    }
    else if (function < PipeWriter_FIRST_PARAM) {
        status = NDPluginFile::writeOctet(pasynUser, value, nChars, nActual);
    }


    return (asynStatus)status;

}

NDPluginPipeWriter::NDPluginPipeWriter(const char *portName, int queueSize,
             int blockingCallbacks, const char *NDArrayPort,
             char *cmdPipeInName, char *cmdPipeOutName, int NDArrayAddr,
             int maxBuffers, size_t maxMemory,
             int priority, int stackSize)
  : NDPluginFile(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_PIPE_WRITER_PARAMS,
                   maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   ASYN_CANBLOCK, 1, priority, stackSize)
{
    isPipeOpen = false;

    createParam(PipeWriter_CommandPipeInString,
            asynParamOctet, &PipeWriter_CommandPipeIn);
    createParam(PipeWriter_CommandPipeOutString,
            asynParamOctet, &PipeWriter_CommandPipeOut);
    createParam(PipeWriter_XSizeString,
            asynParamInt32, &PipeWriter_XSize);
    createParam(PipeWriter_YSizeString,
            asynParamInt32, &PipeWriter_YSize);
    createParam(PipeWriter_DataSourceTypeString,
            asynParamInt32, &PipeWriter_DataSourceType);
    createParam(PipeWriter_InputDataSourceString,
            asynParamOctet, &PipeWriter_InputDataSource);
    createParam(PipeWriter_NumTestImagesString,
            asynParamInt32, &PipeWriter_NumTestImages);
    createParam(PipeWriter_TestImagePeriodString,
            asynParamInt32, &PipeWriter_TestImagePeriod);
    createParam(PipeWriter_InputInfImagesString,
            asynParamInt32, &PipeWriter_InputInfImages);
    createParam(PipeWriter_InputQueueSizeString,
            asynParamInt32, &PipeWriter_InputQueueSize);
    createParam(PipeWriter_OutputQueueSizeString,
            asynParamInt32, &PipeWriter_OutputQueueSize);
    createParam(PipeWriter_InputQueueNumImagesString,
            asynParamInt32, &PipeWriter_InputQueueNumImages);
    createParam(PipeWriter_OutputQueueNumImagesString,
            asynParamInt32, &PipeWriter_OutputQueueNumImages);
    createParam(PipeWriter_ResetQueuesString,
            asynParamInt32, &PipeWriter_ResetQueues);
    createParam(PipeWriter_CalcOutTypeString,
            asynParamInt32, &PipeWriter_CalcOutType);
    createParam(PipeWriter_OutputFileTypeString,
            asynParamInt32, &PipeWriter_OutputFileType);
    createParam(PipeWriter_OutputFilePathString,
            asynParamOctet, &PipeWriter_OutputFilePath);
    createParam(PipeWriter_OutputFilePathExistsString,
            asynParamInt32, &PipeWriter_OutputFilePathExists);
    createParam(PipeWriter_OutputFileNameString,
            asynParamOctet, &PipeWriter_OutputFileName);
    createParam(PipeWriter_OutputNCaptureString,
            asynParamInt32, &PipeWriter_OutputNCapture);
    createParam(PipeWriter_OutputNCapturedString,
            asynParamInt32, &PipeWriter_OutputNCaptured);
    createParam(PipeWriter_OutputIncFileNumString,
            asynParamInt32, &PipeWriter_OutputIncFileNum);
    createParam(PipeWriter_OutputNextNumberString,
            asynParamInt32, &PipeWriter_OutputNextNumber);
    createParam(PipeWriter_OutputBigStreamString,
            asynParamInt32, &PipeWriter_OutputBigStream);
    createParam(PipeWriter_OutputCaptureInfString,
            asynParamInt32, &PipeWriter_OutputCaptureInf);
    createParam(PipeWriter_OutputCaptureStartString,
            asynParamInt32, &PipeWriter_OutputCaptureStart);
    createParam(PipeWriter_OutputCaptureStopString,
            asynParamInt32, &PipeWriter_OutputCaptureStop);
    createParam(PipeWriter_OutputCaptureStatusString,
            asynParamInt32, &PipeWriter_OutputCaptureStatus);
    createParam(PipeWriter_RunStateString,
            asynParamInt32, &PipeWriter_RunState);
    createParam(PipeWriter_ProcessStartString,
            asynParamInt32, &PipeWriter_ProcessStart);
    createParam(PipeWriter_ProcessStopString,
            asynParamInt32, &PipeWriter_ProcessStop);
    createParam(PipeWriter_ProcessStatusString,
            asynParamInt32, &PipeWriter_ProcessStatus);


    /* Initialize parameters */
    setStringParam(NDPluginDriverPluginType, "NDPluginPipeWriter");
    setStringParam(PipeWriter_CommandPipeIn, cmdPipeInName);
    setStringParam(PipeWriter_CommandPipeOut, cmdPipeOutName);
    lastFrameNumber = -1;
    this->supportsMultipleArrays = 1;
    isPipeOpen = false;
    cmdInPipeOpen = false;
    cmdOutPipeOpen = false;
    keepGoing = true;

    openCommandOutputPipe();
    openCommandInputPipe();

    /* Create the thread that updates the images */
    int status;
    status = (epicsThreadCreate("watchMPICommandOutputTaskC",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC)watchMPICommandOutputTaskC,
                                this) == NULL);

    epicsAtExit(exitCallbackC, this);
}

NDPluginPipeWriter::~NDPluginPipeWriter(){
    keepGoing = false;
    closeCommandOutputPipe();
    closeCommandInputPipe();
}

/** Configuration command */
extern "C" int NDPipeWriterConfigure(const char *portName, int queueSize,
                                    int blockingCallbacks,
                                    const char *NDArrayPort,
                                    char *cmdPipeInName, char *cmdPipeOutName,
                                    int NDArrayAddr,
                                    int maxBuffers, size_t maxMemory,
                                    int priority, int stackSize)
{
  new NDPluginPipeWriter(portName, queueSize, blockingCallbacks, NDArrayPort,
            cmdPipeInName, cmdPipeOutName, NDArrayAddr,
              maxBuffers, maxMemory, priority, stackSize);
  return(asynSuccess);
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "cmdPipeInName",iocshArgString};
static const iocshArg initArg5 = { "cmdPipeOutName",iocshArgString};
static const iocshArg initArg6 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg7 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg8 = { "maxMemory",iocshArgInt};
static const iocshArg initArg9 = { "priority",iocshArgInt};
static const iocshArg initArg10 = { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8,
                                            &initArg9,
                                            &initArg10};
static const iocshFuncDef initFuncDef = {"NDPipeWriterConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
  NDPipeWriterConfigure(args[0].sval, args[1].ival, args[2].ival,
                       args[3].sval, args[4].sval, args[5].sval,
                       args[6].ival, args[7].ival, args[8].ival,
                       args[9].ival, args[10].ival);
}

extern "C" void NDPipeWriterRegister(void)
{
  iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
  epicsExportRegistrar(NDPipeWriterRegister);
}
