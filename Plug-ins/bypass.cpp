#include "bypass.h"

#if defined (OS_Q_LINUX)
#include <QDBusReply>
#endif

#include <QtDBus/QDBusReply>

#include <QDebug>
#include <QFile>
#include <QFileSystemWatcher>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

AppArmorPolicyBypasser::AppArmorPolicyBypasser(QObject* parent)
  : QObject(parent)
  , m_installerProcess(0)
  , m_download(0)
  , m_bus(QDBusConnection::systemBus())
  , m_iface("com.lomiri.click", "/com/lomiri/click", "com.lomiri.click", m_bus)
{
  m_nam = new QNetworkAccessManager(this);
}

bool AppArmorPolicyBypasser::busy() const
{
  return m_installerProcess != 0 || m_download != 0;
}

bool AppArmorPolicyBypasser::isLocalInstall() const
{
  return m_isLocalInstall;
}

int AppArmorPolicyBypasser::downloadProgress() const
{
  if (m_file.isOpen()) {
    return m_file.size();
  }
  return 0;
}

void AppArmorPolicyBypasser::installPackage(const QString& packageUrl, const bool isLocalInstall)
{
  m_isLocalInstall = isLocalInstall;
  Q_EMIT isLocalInstallChanged();

  // qDebug() << "should install package" << packageUrl;
  if (busy()) {
    // qDebug() << "already busy. won't install" << packageUrl;
    return;
  }

  if (packageUrl.startsWith("http://") || packageUrl.startsWith("https://")) {
    fetchPackage(packageUrl);
    return;
  }

  QString localPath = packageUrl;
  localPath.remove(QRegExp("$file://"));
  installLocalPackage(localPath);
}

void AppArmorPolicyBypasser::removePackage(const QString& appId, const QString& version)
{
  if (m_installerProcess) {
    return;
  }
  qDebug() << "starting package removal:" << appId << version << m_iface.isValid();

  if (m_iface.isValid()) {
    auto reply = m_iface.call("Remove", appId);
    if (reply.type() == QDBusMessage::ErrorMessage) {
      qCritical() << "Error removing package" << appId;
      Q_EMIT packageInstallationFailed();
    } else {
      Q_EMIT packageInstalled();
    }
  } else {
    m_installerProcess = new QProcess(this);
    connect(m_installerProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(installerFinished(int, QProcess::ExitStatus)));
    connect(m_installerProcess, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(processStatusChanged(QProcess::ProcessState)));
    m_installerProcess->start("pkcon", QStringList() << "remove" << appId + ";" + version + ";all;local:click");
  }
  Q_EMIT busyChanged();
}

bool AppArmorPolicyBypasser::abortInstallation() const
{
  if (busy() && m_download) {
    m_download->abort();
    return true;
  }

  // else
  return false;
}

void AppArmorPolicyBypasser::fetchPackage(const QString& packageUrl)
{
  QUrl url(packageUrl);
  qDebug() << "fetching package" << url.url();

  m_file.setFileName("/tmp/" + url.fileName());
  if (!m_file.open(QFile::WriteOnly | QFile::Truncate)) {
    qDebug() << "Cannot open temp file" << m_file.fileName();
    return;
  }

  QNetworkRequest request;
  request.setUrl(url);

  m_download = m_nam->get(request);
  connect(m_download, &QNetworkReply::downloadProgress, this, &AppArmorPolicyBypasser::slotDownloadProgress);
  connect(m_download, &QNetworkReply::finished, this, &AppArmorPolicyBypasser::downloadFinished);

  Q_EMIT busyChanged();
  Q_EMIT downloadProgressChanged();
}

void AppArmorPolicyBypasser::installLocalPackage(const QString& packageFilePath)
{
  if (m_installerProcess) {
    return;
  }
  qDebug() << "starting installer:" << packageFilePath << m_iface.isValid();

  if (m_iface.isValid()) {
    auto reply = m_iface.call("Install", packageFilePath);
    if (reply.type() == QDBusMessage::ErrorMessage) {
      qCritical() << "Error Installing package" << packageFilePath;
      Q_EMIT packageInstallationFailed();
    } else {
      Q_EMIT packageInstalled();
    }
  } else {
    m_installerProcess = new QProcess(this);
    connect(m_installerProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(installerFinished(int, QProcess::ExitStatus)));
    connect(m_installerProcess, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(processStatusChanged(QProcess::ProcessState)));
    m_installerProcess->start("pkcon",
			   QStringList() << "install-local"
				        << "--allow-untrusted" << packageFilePath);
  }
  Q_EMIT busyChanged();
}

void AppArmorPolicyBypasser::installerFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  qDebug() << "installing finished" << exitCode << exitStatus;
  qDebug() << "stdout:" << QString::fromUtf8(m_installerProcess->readAll());
  m_installerProcess->deleteLater();
  m_installerProcess = 0;
  Q_EMIT busyChanged();
  if (exitCode == 0 && exitStatus == 0) {
    Q_EMIT packageInstalled();
  } else {
    Q_EMIT packageInstallationFailed();
  }
}

void AppArmorPolicyBypasser::processStatusChanged(QProcess::ProcessState state)
{
  Q_UNUSED(state);
  //    qDebug() << "process state changed:" << state;
}

void AppArmorPolicyBypasser::slotDownloadProgress()
{
  m_file.write(m_download->readAll());
  //    qDebug() << "downloadProgress" << m_file.size();
  Q_EMIT downloadProgressChanged();
}

void AppArmorPolicyBypasser::downloadFinished()
{
  qDebug() << "finished" << m_download->error();
  if (m_download->error() != QNetworkReply::NoError) {
    qDebug() << m_download->errorString() << m_download->attribute(QNetworkRequest::RedirectionTargetAttribute);
  }

  m_file.write(m_download->readAll());
  m_file.close();

  m_download->deleteLater();

  m_isLocalInstall = false;
  Q_EMIT isLocalInstallChanged();

  if (m_download->error() == QNetworkReply::OperationCanceledError) {
    Q_EMIT downloadProgressChanged();

    m_download = 0;

    Q_EMIT busyChanged();

    return;
  }

  // QByteArray data = reply->readAll();

  if (!m_download->attribute(QNetworkRequest::RedirectionTargetAttribute).toString().isEmpty()) {
    qDebug() << "fetching new url:" << m_download->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
    fetchPackage(m_download->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl().toString());
  } else {
    qDebug() << "Package fetched. Starting installation";
    m_download = 0;
    installLocalPackage(m_file.fileName());
    Q_EMIT downloadProgressChanged();
    Q_EMIT busyChanged();
  }
}
