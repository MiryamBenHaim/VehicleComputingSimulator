#include <QMessageBox>
#include <QHBoxLayout>
#include "process.h"
#include "main_window.h"
#include <QFileInfo>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) 
    : QMainWindow(parent), process1(nullptr), process2(nullptr), timer(nullptr) 
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    QWidget *toolbox = new QWidget(this);
    toolboxLayout = new QVBoxLayout(toolbox);
    startButton = new QPushButton("Start", this);
    endButton = new QPushButton("End", this);
    timerButton = new QPushButton("Set Timer", this);
    timeInput = new QLineEdit(this);
    timeLabel = new QLabel("Enter time in seconds:", this);
    logOutput = new QTextEdit(this);
    QPushButton *chooseButton = new QPushButton("choose img", this);

    timeLabel->hide();
    timeInput->hide();
    logOutput->setReadOnly(true);

    mainLayout->addWidget(toolbox);

    QPushButton *addProcessButton = new QPushButton("Add Process", toolbox);
    toolboxLayout->addWidget(addProcessButton);
    toolboxLayout->addStretch();
    connect(addProcessButton, &QPushButton::clicked, this, &MainWindow::createNewProcess);
    connect(startButton, &QPushButton::clicked, this, &MainWindow::startProcesses);
    connect(endButton, &QPushButton::clicked, this, &MainWindow::endProcesses);
    connect(timerButton, &QPushButton::clicked, this, &MainWindow::showTimerInput);
    connect(chooseButton, &QPushButton::clicked, this, &MainWindow::openImageDialog);

    toolbox->setMaximumWidth(100);
    toolbox->setMinimumWidth(100);
    toolboxLayout->addWidget(startButton);
    toolboxLayout->addWidget(endButton);
    toolboxLayout->addWidget(timerButton);
    toolboxLayout->addWidget(timeLabel);
    toolboxLayout->addWidget(timeInput);
    toolboxLayout->addWidget(logOutput);
    toolboxLayout->addWidget(chooseButton);

    workspace = new QWidget(this);
    workspace->setStyleSheet("background-color: white;");

    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    mainLayout->addWidget(workspace);
    centralWidget->setLayout(mainLayout);

    dataManager = new SimulationDataManager(this);

    int id = 0;
    Process mainProcess(id, "Main", "../src/dummy_program1", "QEMUPlatform");
    addProcessSquare(mainProcess, id,"background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1, stop: 0 #0000FF, stop: 1 #800080);");
    addId(id++);
    Process hsmProcess(id, "HSM", "../src/dummy_program2", "QEMUPlatform");
    addProcessSquare(hsmProcess, id,"background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1, stop: 0 #0000FF, stop: 1 #800080);");
    addId(id++);
    Process logsDbProcess(id, "LogsDb", "../src/dummy_program1", "QEMUPlatform");
    addProcessSquare(logsDbProcess, id,"background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1, stop: 0 #0000FF, stop: 1 #800080);");
    addId(id++);
    Process busManagerProcess(id, "Bus_Manager", "../src/dummy_program2", "QEMUPlatform");
    addProcessSquare(busManagerProcess, id,"background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1, stop: 0 #0000FF, stop: 1 #800080);");
    addId(id++);
}

MainWindow::~MainWindow() 
{
    qDeleteAll(squares);
    if (process1) delete process1;
    if (process2) delete process2;
    if (timer) delete timer;
}

void MainWindow::createNewProcess() 
{
    ProcessDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted && dialog.isValid()) {
        int id = dialog.getId();
        if (id <= 10) {
            QMessageBox::warning(this, "Invalid ID", "The ID must be greater than 10.");
            return;
        }
        if (!isUniqueId(id)) {
            QMessageBox::warning(this, "Non-unique ID", "The ID entered is already in use. Please choose a different ID.");
            return;
        }
        Process newProcess(id, dialog.getName(), dialog.getCMakeProject(), dialog.getQEMUPlatform());
        addProcessSquare(newProcess);
        addId(id);
    }
}

void MainWindow::addProcessSquare(const Process& process) 
{
    DraggableSquare *square = new DraggableSquare(workspace);
    square->setProcess(process);

    QPoint pos = squarePositions.value(process.getId(), QPoint(0, 0));
    square->move(pos);
    square->show();

    squarePositions[process.getId()] = pos;
    squares.push_back(square);
}

void MainWindow::addProcessSquare(const Process& process, int index,const QString &color) 
{
    DraggableSquare *square = new DraggableSquare(workspace,color,120,120);
    square->setProcess(process);

    int x = (index % 2) * (square->width() + 10);
    int y = (index / 2) * (square->height() + 10);
    QPoint pos = squarePositions.value(process.getId(), QPoint(x, y));
    square->move(pos);
    square->show();

    squarePositions[process.getId()] = pos;
    squares.push_back(square);

}

bool MainWindow::isUniqueId(int id) 
{
    return !usedIds.contains(id);
}

void MainWindow::addId(int id) 
{
    usedIds.insert(id);
}

void MainWindow::startProcesses() 
{
    QString inputText = timeInput->text();
    bool ok = true;
    int time = 0;

    if (!inputText.isEmpty()) {
        time = inputText.toInt(&ok);
    }

    if (!ok || (time <= 0 && !inputText.isEmpty())) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a valid number of seconds.");
        timeInput->clear();
        return;
    }

    if (time > 0) {
        logOutput->append("Timer started for " + QString::number(time) + " seconds.");

        if (timer) {
            timer->stop();
            delete timer;
        }

        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &MainWindow::timerTimeout);

        timer->start(time * 1000);
        timeLabel->hide();
        timeInput->hide();
    }



    compileBoxes();
}

void MainWindow::endProcesses() {
    logOutput->append("Ending processes...");

    if (timer) {
        timer->stop();
        delete timer;
        timer = nullptr;
    }

    timeInput->show();
    timeLabel->show();
    timeInput->clear();

    dataManager->saveSimulationData("simulation_data.bson", squares, currentImagePath);

    
    QString filePath = "../log_file.log";
    logHandler.readLogFile(filePath);
    logHandler.analyzeLogEntries(this,"simulation_data.bson");

    for (QProcess* process : runningProcesses) {
        if (process->state() != QProcess::NotRunning) {
            logOutput->append("Ending process...");
            process->terminate();
            process->waitForFinished();
        }
        delete process;
    }
    runningProcesses.clear();
}

void MainWindow::showTimerInput() 
{
    timeLabel->show();
    timeInput->show();
}

void MainWindow::timerTimeout() 
{
    logOutput->append("Timer timeout reached.");
    endProcesses();
}

void MainWindow::readProcess1Output() 
{
    logOutput->append("Process1: " + process1->readAllStandardOutput());
}

void MainWindow::readProcess2Output() 
{
    logOutput->append("Process2: " + process2->readAllStandardOutput());
}

void MainWindow::openImageDialog() 
{
    QString imagePath = QFileDialog::getOpenFileName(this, tr("Select Image"), "", tr("Image Files (*.png *.jpg *.jpeg)"));
    if (!imagePath.isEmpty()) {
        currentImagePath = imagePath;
        QPixmap pixmap(imagePath);
        if (!pixmap.isNull()) {
            // Clear the workspace before adding the new image
            QLayout *layout = workspace->layout();
            if (layout) {
                QLayoutItem *item;
                while ((item = layout->takeAt(0)) != nullptr) {
                    delete item->widget();  // Delete the widget
                    delete item;            // Delete the layout item
                }
            }

            // Create a new QLabel to display the image as background
            QLabel *backgroundLabel = new QLabel(workspace);
            backgroundLabel->setPixmap(pixmap.scaled(workspace->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            backgroundLabel->setGeometry(workspace->rect()); // Fit QLabel to the size of the workspace
            backgroundLabel->setScaledContents(true);
            backgroundLabel->setAttribute(Qt::WA_TranslucentBackground); // Ensure the image appears only as background
            backgroundLabel->lower(); // Move QLabel below other content in the workspace
            backgroundLabel->show();

            // Add a new layout to the workspace
            QVBoxLayout *newLayout = new QVBoxLayout(workspace);
            workspace->setLayout(newLayout);
        }
    }
}


QString MainWindow::getExecutableName(const QString &buildDirPath) 
{
    QDir buildDir(buildDirPath);

    // List all files in the build directory
    QStringList files = buildDir.entryList(QDir::Files | QDir::NoSymLinks);

    // Iterate over the files to find the executable
    foreach (const QString &file, files) {
        QFileInfo fileInfo(buildDir.filePath(file));
        
        // Check if the file has no extension (common on Linux) or is an executable (on Windows)
        if (!fileInfo.suffix().isEmpty()) continue;

        // Check if the file has execute permissions (Unix-based systems)
        if (fileInfo.isExecutable()) {
            return fileInfo.fileName();
        }
    }

    // If no executable file is found, return an empty string
    return QString();
}

void MainWindow::compileBoxes() {
    // Clear previous running processes
    for (QProcess* process : runningProcesses) {
        process->terminate();
        process->waitForFinished();
        delete process;
    }
    runningProcesses.clear();

    // Iterate over each directory and compile
    for (const DraggableSquare* square : squares) {
        QString cmakePath = square->getProcess().getCMakeProject();
        logOutput->append("Compiling " + cmakePath);

        // Define the build directory path
        QDir cmakeDir(cmakePath);
        QString buildDirPath = cmakeDir.absoluteFilePath("build");

        // Create the build directory if it doesn't exist
        QDir buildDir(buildDirPath);
        if (!buildDir.exists()) {
            if (!buildDir.mkpath(".")) {
                logOutput->append("Failed to create build directory " + buildDirPath);
                continue;
            }
        }
        // Clean the build directory
        QProcess cleanProcess(this);
        cleanProcess.setWorkingDirectory(buildDirPath);
        cleanProcess.start("rm", QStringList() << "-rf" << "*");
        if (!cleanProcess.waitForFinished()) {
            logOutput->append("Failed to clean build directory " + buildDirPath);
            continue;
        }
        QProcess* cmakeProcess = new QProcess(this);
        cmakeProcess->setWorkingDirectory(buildDirPath);
        cmakeProcess->start("cmake", QStringList() << "..");
        if (!cmakeProcess->waitForFinished()) {
            logOutput->append("Failed to run cmake in " + buildDirPath);
            delete cmakeProcess;
            continue;
        }
        logOutput->append(cmakeProcess->readAllStandardOutput());
        logOutput->append(cmakeProcess->readAllStandardError());
        delete cmakeProcess;

        QProcess* makeProcess = new QProcess(this);
        makeProcess->setWorkingDirectory(buildDirPath);
        makeProcess->start("make", QStringList());
        if (!makeProcess->waitForFinished()) {
            logOutput->append("Failed to compile " + buildDirPath);
            delete makeProcess;
            continue;
        }
        logOutput->append(makeProcess->readAllStandardOutput());
        logOutput->append(makeProcess->readAllStandardError());
        delete makeProcess;

        logOutput->append("Successfully compiled " + buildDirPath);

        // Run the compiled program
        QString exeFile = getExecutableName(buildDirPath);
        QString executablePath = buildDir.absoluteFilePath(exeFile);
        QProcess* runProcess = new QProcess(this);
        runProcess->setWorkingDirectory(buildDirPath);

        connect(runProcess, &QProcess::readyReadStandardOutput, [this, runProcess]() {
            logOutput->append(runProcess->readAllStandardOutput());
        });

        connect(runProcess, &QProcess::readyReadStandardError, [this, runProcess]() {
            logOutput->append(runProcess->readAllStandardError());
        });

        runProcess->start(executablePath, QStringList());
        runningProcesses.append(runProcess);
    }
}


// Include the generated moc file
#include "moc_main_window.cpp"