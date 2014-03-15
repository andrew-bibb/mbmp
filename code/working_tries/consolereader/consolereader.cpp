
# include <QTextStream>

# include <unistd.h> //Provides STDIN_FILENO

# include "consolereader.h"

ConsoleReader::ConsoleReader(QObject *parent) :
    QObject(parent)
{
    notifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read);
    connect(notifier, SIGNAL(activated(int)), this, SLOT(text()));
}

void ConsoleReader::text()
{
    QTextStream qin(stdin);
    QString line = qin.readLine();
    emit textReceived(line);
    
    QTextStream qout(stdout);
    qout << "hi andy";
}
