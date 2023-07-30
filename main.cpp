#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QStringList>
#include <QDebug>

struct FunctionInfo {
    QString name;
    QString returnType;
    QStringList args;
    bool isVirtual;
    bool isInline;
};
QStringList extractClassNames(const QString& headerContent);
QString generateMockClass(const QString& className, const QString& classContent);
QStringList extractMethodSignature(const QString& className, const QString& classContent);
QString extractFunctionName(const QString& methodSignature);
QString extractReturnType(const QString& methodSignature);
QString extractArgs(const QString& methodSignature);
QStringList parseArgs(const QString& args);
QString genMockClass(const QString& functionName, const QString& returnType, const QStringList& args);
FunctionInfo parseFunction(const QString& functionSignature);
QString parseClassContent(const QString& fileContent, const QString& className);

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    if (argc != 3) {
        qDebug() << "Usage: mockgen input_file.h output_file.h";
        return 1;
    }

    QString inputFileName = argv[1];
    QString outputFileName = argv[2];

    QFile inputFile(inputFileName);
    if (!inputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error: Cannot open input file:" << inputFileName;
        return 1;
    }

    QString headerContent = QString::fromUtf8(inputFile.readAll());
    inputFile.close();

    // Tìm tất cả các class được định nghĩa trong file .h
    QStringList classNames = extractClassNames(headerContent);
    // Tạo các mock class và ghi vào file .h đầu ra
    QFile outputFile(outputFileName);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Error: Cannot open output file:" << outputFileName;
        return 1;
    }

    QTextStream out(&outputFile);
    for (const QString& className : classNames) {
        QString classContent = parseClassContent(headerContent,className);
        qDebug() << className<< classContent;
        qDebug() << classContent;
//        classContent = classContent.mid(classContent.indexOf("class " + className));

        QString mockClass = generateMockClass(className, classContent);
        out << mockClass << "\n";
    }

    outputFile.close();

    qDebug() << "Mock classes generated successfully in" << outputFileName;

    return 0;
}

QString parseClassContent(const QString& fileContent, const QString& className) {
    QString classContent;
    QRegularExpression classRegex(R"(class\s+)" + className + R"(\s*{)");

    QRegularExpressionMatch match = classRegex.match(fileContent);
    if (match.hasMatch()) {
        int startPos = match.capturedEnd();
        int endPos = startPos;
        int openBrackets = 1;

        while (endPos < fileContent.length() && openBrackets > 0) {
            QChar currentChar = fileContent.at(endPos);

            if (currentChar == '{') {
                openBrackets++;
            } else if (currentChar == '}') {
                openBrackets--;
            }

            endPos++;
        }

        if (openBrackets == 0) {
            classContent = fileContent.mid(startPos, endPos - startPos - 1).trimmed();
        }
    }

    return classContent;
}

QStringList extractClassNames(const QString& headerContent) {
    QRegularExpression classRegex(R"(class\s+(\w+)\s*[:{])");
    QRegularExpressionMatchIterator regexIterator = classRegex.globalMatch(headerContent);

    QStringList classNames;
    while (regexIterator.hasNext()) {
        QRegularExpressionMatch match = regexIterator.next();
        QString className = match.captured(1).trimmed();
        classNames.append(className);
    }

    return classNames;
}

QString generateMockClass(const QString& className, const QString& classContent) {
    QString mockClass;
    mockClass += "#include <gmock/gmock.h>\n";
    mockClass += QString("class Mock%1 : public %1{\n").arg(className);
//    qDebug() << className << classContent;
    // Thêm các phương thức của class gốc dưới dạng ảo và trả về giá trị mặc định
    mockClass += "public:\n";

    QStringList methodSignatures = extractMethodSignature(className, classContent);
    foreach(QString methodSignature , methodSignatures) {

        //        qInfo() << methodSignature;
        if (!methodSignature.isEmpty()) {
            FunctionInfo funcInfo = parseFunction(methodSignature);
            if (!funcInfo.name.isEmpty() && !funcInfo.returnType.isEmpty()) {
                // Thêm mock method tương ứng
                mockClass += genMockClass(funcInfo.name,funcInfo.returnType,funcInfo.args);
//                qDebug() <<genMockClass(funcInfo.name,funcInfo.returnType,funcInfo.args);
            }
        }

    }

    mockClass += "};\n";
    return mockClass;
}

FunctionInfo parseFunction(const QString& functionSignature) {
    FunctionInfo functionInfo;

    QString functionName = extractFunctionName(functionSignature);
    QString returnType = extractReturnType(functionSignature);
    QString args = extractArgs(functionSignature);
    QStringList argList = parseArgs(args);

    functionInfo.name = functionName;
    functionInfo.returnType = returnType;
    functionInfo.args = argList;
    functionInfo.isVirtual = functionSignature.contains("virtual");
    functionInfo.isInline = functionSignature.contains("inline");

    return functionInfo;
}

QStringList extractMethodSignature(const QString& className, const QString& classContent) {
    qDebug() ;
    QStringList methodSignatures;
    QStringList delimiters = {";", "}", ":"};
    int startPos = 0;

    while (startPos < classContent.length()) {
        // Tìm vị trí kết thúc của các đoạn dựa trên các ký hiệu ";", "}", ":"
        int endPos = classContent.length();
        for (const QString& delimiter : delimiters) {
            int delimiterPos = classContent.indexOf(delimiter, startPos);
            if (delimiterPos != -1 && delimiterPos < endPos) {
                endPos = delimiterPos;
                if(delimiter=="}") endPos++;
            }
        }

        QString methodSignature = classContent.mid(startPos, endPos - startPos).trimmed();
        if (!methodSignature.isEmpty() && methodSignature.contains('(') && methodSignature.contains(')')) {
            methodSignatures.append(methodSignature);
        }

        startPos = endPos + 1;
    }

    return methodSignatures;
}



QString extractFunctionName(const QString& methodSignature) {
    int bracketPos = methodSignature.indexOf('(');
    if (bracketPos == -1) {
        return "";
    }

    QString returnTypePart = methodSignature.left(bracketPos).trimmed();
    QStringList returnTypeParts = returnTypePart.split(' ', Qt::SkipEmptyParts);
    if (returnTypeParts.isEmpty()) {
        return "";
    }

    QString functionName = returnTypeParts.last();
    return functionName;;
}

QString extractReturnType(const QString& methodSignature) {
    int bracketPos = methodSignature.indexOf('(');
    if (bracketPos == -1) {
        return "";
    }

    QString returnTypePart = methodSignature.left(bracketPos).trimmed();
    returnTypePart.remove("virtual");
    returnTypePart.remove("inline");
    QStringList returnTypeParts = returnTypePart.split(' ', Qt::SkipEmptyParts);
    if (returnTypeParts.isEmpty()) {
        return "";
    }

    returnTypeParts.removeLast(); // Loại bỏ function name
    QString returnType = returnTypeParts.join(' ');
    return returnType;
}

QString extractArgs(const QString& methodSignature) {
    QRegularExpression argsRegex(R"(\((.*)\))");
    QRegularExpressionMatch match = argsRegex.match(methodSignature);
    return match.hasMatch() ? match.captured(1) : "";
}

QStringList parseArgs(const QString& args) {
    QStringList argList = args.split(",", Qt::SkipEmptyParts);
    return argList;
}

QString genMockClass(const QString& functionName, const QString& returnType, const QStringList& args) {
    QString mockMethod;
    mockMethod += "    MOCK_METHOD" + QString::number(args.length()) + "(" + functionName + ", " + returnType + "(";
    if (!args.isEmpty()) {
        mockMethod += args.join(", ") + "))";
    } else {
        mockMethod += "))";
    }
    mockMethod += ";\n";
    //    mockMethod += (returnType.isEmpty() ? "" : " " + returnType) + ";";

    return mockMethod;
}





