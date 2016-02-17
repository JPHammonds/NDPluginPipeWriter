#ifndef NDPluginPipeWriter_H
#define NDPluginPipeWriter_H
#include <string>
#include <epicsTypes.h>
#include <asynStandardInterfaces.h>
#include "NDPluginFile.h"

using std::string;


class epicsShareClass NDPluginPipeWriter : public NDPluginFile {
public:
    static const char* pluginName;
    static const char* mpiProgName;
    static string intStr;
    static string boolStr;
    static string qstringStr;
    static string windowStr;
    static string xSizeCmd;
    static string ySizeCmd;
    static string dataSourceTestImagesCmd;
    static string dataSourceLinuxPipeCmd;
    static string numTestImagesCmd;
    static string testImagePeriodCmd;
    static string infiniteInImagesCmd;
    static string descrambleSelectCmd;
    static string nToAvgCmd;
    static string startClickedCmd;
    static string stopClickedCmd;
    static string acqDarksClickedCmd;
    static string subDarksClickedCmd;
    static string inQueueLenChangedCmd;
    static string outQueueLenChangedCmd;
    static string resetQueuesClickedCmd;
    static string tiffNumChangedCmd;
    static string nullOutputSelectedCmd;
    static string tiffOutputSelectedCmd;
    static string immOutputSelectedCmd;
    static string linuxPipeOutSelectedCmd;
    static string rawImgOutSelectedCmd;
    static string rawImmOutSelectedCmd;
    static string compImmOutSelectedCmd;
    static string num2CaptureChangedCmd;
    static string outFilePathChangedCmd;
    static string outFileBaseNameChangedCmd;
    static string outIncFileNumCmd;
    static string outNextNumberCmd;
    static string emptyStr;
    NDPluginPipeWriter(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort,
                 char *cmdPipeInName, char *cmdPipeOutName, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    ~NDPluginPipeWriter();
        /* These methods override the virtual methods in the base class */
    //void processCallbacks(NDArray *pArray);
    virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray);
    virtual asynStatus readFile(NDArray **pArray);
    virtual asynStatus writeFile(NDArray *pArray);
    virtual asynStatus closeFile();
    void watchMPICommandOutputTask();
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual);

protected:
    int PipeWriter_CommandPipeIn;
#define PipeWriter_FIRST_PARAM PipeWriter_CommandPipeIn
    int PipeWriter_CommandPipeOut;
    int PipeWriter_XSize;
    int PipeWriter_YSize;
    int PipeWriter_DataSourceType;
    int PipeWriter_InputDataSource;
    int PipeWriter_NumTestImages;
    int PipeWriter_TestImagePeriod;
    int PipeWriter_InputInfImages;
    int PipeWriter_InputQueueSize;
    int PipeWriter_OutputQueueSize;
    int PipeWriter_InputQueueNumImages;
    int PipeWriter_OutputQueueNumImages;
    int PipeWriter_ResetQueues;
    int PipeWriter_CalcOutType;
    int PipeWriter_OutputFileType;
    int PipeWriter_OutputFilePath;
    int PipeWriter_OutputFilePathExists;
    int PipeWriter_OutputFileName;
    int PipeWriter_OutputFullFileName;
    int PipeWriter_OutputNCapture;
    int PipeWriter_OutputNCaptured;
    int PipeWriter_OutputIncFileNum;
    int PipeWriter_OutputNextNumber;
    int PipeWriter_OutputNextNumberRBV;
    int PipeWriter_OutputBigStream;
    int PipeWriter_OutputCaptureInf;
    int PipeWriter_OutputCaptureStart;
    int PipeWriter_OutputCaptureStop;
    int PipeWriter_OutputCaptureStatus;
    int PipeWriter_RunState;
    int PipeWriter_ProcessStart;
    int PipeWriter_ProcessStop;
    int PipeWriter_ProcessStatus;
#define PipeWriter_LAST_PARAM PipeWriter_ProcessStatus

private:



    FILE *inFile;
    bool isPipeOpen;
    bool cmdInPipeOpen;
    bool cmdOutPipeOpen;
    int lastFrameNumber;
    char *cmdPipeInName;
    char *cmdPipeOutName;
    FILE *cmdInPipe;
    FILE *cmdOutPipe;
    asynStatus checkOutputPath();
    bool closeCommandInputPipe();
    bool closeCommandOutputPipe();
    bool keepGoing;
    bool openCommandInputPipe();
    bool openCommandOutputPipe();
    void sendCommand(const string progName, const string slotName, const string argName, int numArgs, const string argType, std::string args);

};
#define PipeWriter_CommandPipeInString           "PW_CommandPipeIn"
#define PipeWriter_CommandPipeOutString          "PW_CommandPipeOut"
#define PipeWriter_XSizeString                   "PW_XSize"
#define PipeWriter_YSizeString                   "PW_YSize"
#define PipeWriter_DataSourceTypeString          "PW_DataSourceType"
#define PipeWriter_NumTestImagesString           "PW_NumTestImages"
#define PipeWriter_TestImagePeriodString         "PW_TestImagePeriod"
#define PipeWriter_InputInfImagesString          "PW_InputInfImages"
#define PipeWriter_InputQueueSizeString          "PW_InputQueueSize"
#define PipeWriter_OutputQueueSizeString         "PW_OutputQueueSize"
#define PipeWriter_InputQueueNumImagesString     "PW_InputQueueNumImgs"
#define PipeWriter_OutputQueueNumImagesString    "PW_OutputQueueNumImgs"
#define PipeWriter_InputDataSourceString         "PW_InputDataSource"
#define PipeWriter_ResetQueuesString             "PW_ResetQueues"
#define PipeWriter_CalcOutTypeString             "PW_CalcOutType"
#define PipeWriter_OutputFileTypeString          "PW_OutputFileType"
#define PipeWriter_OutputFilePathString          "PW_OutputFilePath"
#define PipeWriter_OutputFilePathExistsString    "PW_OutputFilePathExists"
#define PipeWriter_OutputFileNameString          "PW_OutputFileName"
#define PipeWriter_OutputFullFileNameString      "PW_OutputFullFileName"
#define PipeWriter_OutputNCaptureString          "PW_OutputNCapture"
#define PipeWriter_OutputNCapturedString         "PW_OutputNCaptured"
#define PipeWriter_OutputIncFileNumString        "PW_OutputIncFileNum"
#define PipeWriter_OutputNextNumberString        "PW_OutputNextNumber"
#define PipeWriter_OutputNextNumberRBVString     "PW_OutputNextNumberRBV"
#define PipeWriter_OutputBigStreamString         "PW_OutputBigStream"
#define PipeWriter_OutputCaptureInfString        "PW_OutputCaptureInf"
#define PipeWriter_OutputCaptureStartString      "PW_CaptureStart"
#define PipeWriter_OutputCaptureStopString       "PW_CaptureStop"
#define PipeWriter_OutputCaptureStatusString     "PW_CaptureStatus"
#define PipeWriter_ProcessStartString            "PW_ProcessStart"
#define PipeWriter_ProcessStopString             "PW_ProcessStop"
#define PipeWriter_ProcessStatusString           "PW_ProcessStatus"
#define PipeWriter_RunStateString                "PW_RunState"


#define NUM_PIPE_WRITER_PARAMS ((int)(&PipeWriter_LAST_PARAM - &PipeWriter_FIRST_PARAM+1))

#endif
