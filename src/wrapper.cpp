/**
 * Copyright (c) 2002-2011 "Neo Technology,"
 * Network Engine for Objects in Lund AB [http://neotechnology.com]
 *
 * This file is part of Neo4j.
 *
 * Neo4j is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/app.h>
#include <wx/process.h>
#include <wx/timer.h>
#include <wx/wfstream.h>
#include <iostream>

class NeoWrapper: public wxApp
{
public:
  NeoWrapper()
  {
    m_vendorName = _("Neo Technology");
    m_appName = _("Neo4j Server Wrapper");
  }
  virtual bool OnInit();
  void OnProcessExit(wxProcessEvent& event);
  void OnTimer(wxTimerEvent& event);
  void OnSignal(int signal);
private:
  int pid;
  wxProcess* proc;
  wxInputStream* procout;
  wxInputStream* procerr;
  wxOutputStream* procin;
  wxOutputStream* stdout;
  wxOutputStream* stderr;
  wxInputStream* stdin;
  wxTimer* timer;
  wxChar buffer[4096];

  void SetupSignals();
  void PollStreams();
  bool StartProcess();
  void PipeStream(wxInputStream* source, wxOutputStream* target);
protected:
  DECLARE_EVENT_TABLE()
};

IMPLEMENT_APP_CONSOLE(NeoWrapper);

void signal_handler(int sig)
{
  wxGetApp().OnSignal(sig);
}

BEGIN_EVENT_TABLE(NeoWrapper, wxApp)
  EVT_END_PROCESS(wxID_ANY, NeoWrapper::OnProcessExit)
  EVT_TIMER(wxID_ANY, NeoWrapper::OnTimer)
END_EVENT_TABLE()

void NeoWrapper::SetupSignals()
{
  signal(SIGTERM, signal_handler);
  signal(SIGABRT, signal_handler);
  signal(SIGINT, signal_handler);
}

void NeoWrapper::OnSignal(int sig)
{
  std::cout << "signal: " << sig << std::endl;
}

bool NeoWrapper::OnInit()
{
  stdin = new wxFileInputStream(STDIN_FILENO);
  stdout = new wxFileOutputStream(STDOUT_FILENO);
  stderr = new wxFileOutputStream(STDERR_FILENO);
  if (true/*wxApp::OnInit()*/) {
    //SetupSignals();
    timer = new wxTimer(this);
    if (StartProcess()) {
      PollStreams();
      timer->Start(1);
      return true;
    }
  }
  return false;
}

void NeoWrapper::OnTimer(wxTimerEvent& event)
{
  PollStreams();
}

bool NeoWrapper::StartProcess()
{
  proc = new wxProcess(this);
  proc->Redirect();
  // XXX: not sure if argv is guaranteed to be null terminated
  // we also need a way to update argv so that we can reconfigure and restart
  if ((pid = ::wxExecute((argv+1), wxEXEC_ASYNC, proc)) != 0) {
    if((procin = proc->GetOutputStream())==NULL)
      std::cerr << "cannot redirect to stdin of subprocess" << std::endl;
    if((procout = proc->GetInputStream())==NULL)
      std::cerr << "cannot redirect stdout of subprocess" << std::endl;
    if((procerr = proc->GetErrorStream())==NULL)
      std::cerr << "cannot redirect stderr of subprocess" << std::endl;
    return true;
  }
  return false;
}

void NeoWrapper::PipeStream(wxInputStream* source, wxOutputStream* target)
{
  if (source != NULL && target != NULL && source->CanRead()) {
    size_t size = source->Read(buffer, WXSIZEOF(buffer) - 1).LastRead();
    target->Write(buffer, size);
  }
}

void NeoWrapper::PollStreams()
{
  PipeStream(procerr, stderr);
  PipeStream(procout, stdout);
  //PipeStream(stdin, procin); // <- problematic...
}

void NeoWrapper::OnProcessExit(wxProcessEvent& event)
{
  PollStreams();
  procerr = NULL;
  procout = NULL;
  procin = NULL;
  delete proc; proc = NULL;

  if (event.GetExitCode()) {
    if (StartProcess()) {
      PollStreams();
      return;
    } else {
      std::cout << "process restart failed" << std::endl;
    }
  }
  ExitMainLoop();
}
