#include "common/IDebugLog.h"
#include <share.h>
#include "common/IFileStream.h"
#include <shlobj.h>

std::FILE			* IDebugLog::logFile = NULL;
char				IDebugLog::sourceBuf[16] = { 0 };
char				IDebugLog::headerText[16] = { 0 };
char				IDebugLog::formatBuf[8192] = { 0 };
int					IDebugLog::indentLevel = 0;
int					IDebugLog::rightMargin = 0;
int					IDebugLog::cursorPos = 0;
int					IDebugLog::inBlock = 0;
bool				IDebugLog::autoFlush = true;
IDebugLog::LogLevel	IDebugLog::logLevel = IDebugLog::kLevel_DebugMessage;
IDebugLog::LogLevel	IDebugLog::printLevel = IDebugLog::kLevel_Message;

IDebugLog::IDebugLog()
{
	//
}

IDebugLog::IDebugLog(const char * name)
{
	Open(name);
}

IDebugLog::~IDebugLog()
{
	if(logFile)
		fclose(logFile);
}

void IDebugLog::Open(const char * path)
{
	logFile = _fsopen(path, "w", _SH_DENYWR);

	if(!logFile)
	{
		UInt32	id = 0;
		char	name[1024];

		do
		{
			sprintf_s(name, sizeof(name), "%s%d", path, id);
			id++;

			logFile = NULL;
			logFile = _fsopen(name, "w", _SH_DENYWR);
		}
		while(!logFile && (id < 5));
	}
}

void IDebugLog::OpenRelative(int folderID, const char * relPath)
{
	char	path[MAX_PATH];

	HRESULT err = SHGetFolderPath(NULL, folderID | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, path);
	if(!SUCCEEDED(err))
	{
		_FATALERROR("SHGetFolderPath %08X failed (result = %08X lasterr = %08X)", folderID, err, GetLastError());
	}
	ASSERT_CODE(SUCCEEDED(err), err);

	strcat_s(path, sizeof(path), relPath);

	IFileStream::MakeAllDirs(path);

	Open(path);
}

/**
 *	Output a non-formatted message to the log file
 *	
 *	@param message the message
 *	@param source the source of the message, or NULL to use the previous source
 */
void IDebugLog::Message(const char * message, const char * source, bool newLine)
{
	if(source)
		SetSource(source);

	if(inBlock)
	{
		SeekCursor(RoundToTab((indentLevel * 4) + strlen(headerText)));
	}
	else
	{
		SeekCursor(indentLevel * 4);

		PrintText(headerText);
	}

	PrintText(message);

	if(newLine)
		NewLine();
}

/**
 *	Output a formatted message to the log file
 *	
 *	@note It is impossible to set the source of a formatted message.
 *	The previous source will be used.
 */
void IDebugLog::FormattedMessage(const char * fmt, ...)
{
	va_list	argList;

	va_start(argList, fmt);
	vsprintf_s(formatBuf, sizeof(formatBuf), fmt, argList);
	Message(formatBuf);
	va_end(argList);
}

/**
 *	Output a formatted message to the log file
 *	
 *	@note It is impossible to set the source of a formatted message.
 *	The previous source will be used.
 */
void IDebugLog::FormattedMessage(const char * fmt, va_list args)
{
	vsprintf_s(formatBuf, sizeof(formatBuf), fmt, args);
	Message(formatBuf);
}

void IDebugLog::Log(LogLevel level, const char * fmt, va_list args)
{
	bool	log = (level <= logLevel);
	bool	print = (level <= printLevel);

	if(log || print)
		vsprintf_s(formatBuf, sizeof(formatBuf), fmt, args);

	if(log)
		Message(formatBuf);
	
	if(print)
		printf("%s\n", formatBuf);
}

void IDebugLog::LogNNL(LogLevel level, const char * fmt, va_list args)
{
	bool	log = (level <= logLevel);
	bool	print = (level <= printLevel);

	if(log || print)
		vsprintf_s(formatBuf, sizeof(formatBuf), fmt, args);

	if(log)
		Message(formatBuf, NULL, false);
	
	if(print)
		printf("%s", formatBuf);
}

/**
 *	Set the current message source
 */
void IDebugLog::SetSource(const char * source)
{
	strcpy_s(sourceBuf, sizeof(sourceBuf), source);
	strcpy_s(headerText, sizeof(headerText), "[        ]\t");
	
	char	* tgt = headerText + 1;
	char	* src = sourceBuf;

	for(int i = 0; (i < 8) && *src; i++, tgt++, src++)
		*tgt = *src;
}

/**
 *	Clear the current message source
 */
void IDebugLog::ClearSource(void)
{
	sourceBuf[0] = 0;
}

/**
 *	Increase the indentation level
 */
void IDebugLog::Indent(void)
{
	indentLevel++;
}

/**
 *	Decrease the indentation level
 */
void IDebugLog::Outdent(void)
{
	if(indentLevel)
		indentLevel--;
}

/**
 *	Enter a logical block
 */
void IDebugLog::OpenBlock(void)
{
	SeekCursor(indentLevel * 4);

	PrintText(headerText);

	inBlock = 1;
}

/**
 *	Close a logical block
 */
void IDebugLog::CloseBlock(void)
{
	inBlock = 0;
}

/**
 *	Enable/disable autoflush
 *	
 *	@param inAutoFlush autoflush state
 */
void IDebugLog::SetAutoFlush(bool inAutoFlush)
{
	autoFlush = inAutoFlush;
}

/**
 *	Print spaces to the log
 *	
 *	If possible, tabs are used instead of spaces.
 */
void IDebugLog::PrintSpaces(int numSpaces)
{
	int	originalNumSpaces = numSpaces;

	if(logFile)
	{
		while(numSpaces > 0)
		{
			if(numSpaces >= TabSize())
			{
				numSpaces -= TabSize();
				fputc('\t', logFile);
			}
			else
			{
				numSpaces--;
				fputc(' ', logFile);
			}
		}
	}

	cursorPos += originalNumSpaces;
}

/**
 *	Prints raw text to the log file
 */
void IDebugLog::PrintText(const char * buf)
{
	if(logFile)
	{
		fputs(buf, logFile);
		if(autoFlush)
			fflush(logFile);
	}

	const char	* traverse = buf;
	char		data;

	while(data = *traverse++)
	{
		if(data == '\t')
			cursorPos += TabSize();
		else
			cursorPos++;
	}
}

/**
 *	Moves to the next line of the log file
 */
void IDebugLog::NewLine(void)
{
	if(logFile)
	{
		fputc('\n', logFile);

		if(autoFlush)
			fflush(logFile);
	}

	cursorPos = 0;
}

/**
 *	Prints spaces to align the cursor to the requested position
 *	
 *	@note The cursor move will not be performed if the request would move the cursor
 *	backwards.
 */
void IDebugLog::SeekCursor(int position)
{
	if(position > cursorPos)
		PrintSpaces(position - cursorPos);
}

/**
 *	Returns the number of spaces a tab would occupy at the current cursor position
 */
int IDebugLog::TabSize(void)
{
	return ((~cursorPos) & 3) + 1;
}

/**
 *	Rounds a number of spaces to the nearest tab
 */
int IDebugLog::RoundToTab(int spaces)
{
	return (spaces + 3) & ~3;
}
