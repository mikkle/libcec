/*
 * This file is part of the libCEC(R) library.
 *
 * libCEC(R) is Copyright (C) 2011-2012 Pulse-Eight Limited.  All rights reserved.
 * libCEC(R) is an original work, containing original code.
 *
 * libCEC(R) is a trademark of Pulse-Eight Limited.
 *
 * This program is dual-licensed; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Alternatively, you can license this library under a commercial license,
 * please contact Pulse-Eight Licensing for more information.
 *
 * For more information contact:
 * Pulse-Eight Licensing       <license@pulse-eight.com>
 *     http://www.pulse-eight.com/
 *     http://www.pulse-eight.net/
 */

#include "env.h"
#include <stdio.h>
#include <fcntl.h>
#include "lib/platform/sockets/serialport.h"
#include "lib/platform/util/baudrate.h"
#include "lib/platform/posix/os-socket.h"

#if defined(__APPLE__) || defined(__FreeBSD__)
#ifndef XCASE
#define XCASE	0
#endif
#ifndef OLCUC
#define OLCUC	0
#endif
#ifndef IUCLC
#define IUCLC	0
#endif
#else
#ifdef HAVE_LOCKDEV
#include <lockdev.h>
#endif
#endif

using namespace std;
using namespace PLATFORM;

inline bool RemoveLock(const char *strDeviceName)
{
  #if !defined(__APPLE__) && !defined(__FreeBSD__) && defined(HAVE_LOCKDEV)
  return dev_unlock(strDeviceName, 0) == 0;
  #else
  void *tmp = (void*)strDeviceName; // silence unused warning
  tmp = NULL;
  return true;
  #endif
}

void CSerialSocket::Close(void)
{
  if (IsOpen())
  {
    SocketClose(m_socket);
    RemoveLock(m_strName.c_str());
  }
}

void CSerialSocket::Shutdown(void)
{
  if (IsOpen())
  {
    SocketClose(m_socket);
    RemoveLock(m_strName.c_str());
  }
}

ssize_t CSerialSocket::Write(void* data, size_t len)
{
  return IsOpen() ? SocketWrite(m_socket, &m_iError, data, len) : -1;
}

ssize_t CSerialSocket::Read(void* data, size_t len, uint64_t iTimeoutMs /* = 0 */)
{
  return IsOpen() ? SocketRead(m_socket, &m_iError, data, len, iTimeoutMs) : -1;
}

//setting all this stuff up is a pain in the ass
bool CSerialSocket::Open(uint64_t iTimeoutMs /* = 0 */)
{
  iTimeoutMs = 0; if (!iTimeoutMs){} // silence unused warning
  if (IsOpen())
  {
    m_iError = EINVAL;
    return false;
  }

  if (m_iDatabits != SERIAL_DATA_BITS_FIVE && m_iDatabits != SERIAL_DATA_BITS_SIX &&
      m_iDatabits != SERIAL_DATA_BITS_SEVEN && m_iDatabits != SERIAL_DATA_BITS_EIGHT)
  {
    m_strError = "Databits has to be between 5 and 8";
    m_iError = EINVAL;
    return false;
  }

  if (m_iStopbits != SERIAL_STOP_BITS_ONE && m_iStopbits != SERIAL_STOP_BITS_TWO)
  {
    m_strError = "Stopbits has to be 1 or 2";
    m_iError = EINVAL;
    return false;
  }

  if (m_iParity != SERIAL_PARITY_NONE && m_iParity != SERIAL_PARITY_EVEN && m_iParity != SERIAL_PARITY_ODD)
  {
    m_strError = "Parity has to be none, even or odd";
    m_iError = EINVAL;
    return false;
  }

  #if !defined(__APPLE__) && !defined(__FreeBSD__) && defined(HAVE_LOCKDEV)
  if (dev_lock(m_strName.c_str()) != 0)
  {
    m_strError = "Couldn't lock the serial port";
    m_iError = EBUSY;
    return false;
  }
  #endif

  m_socket = open(m_strName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);

  if (m_socket == INVALID_SERIAL_SOCKET_VALUE)
  {
    m_strError = strerror(errno);
    RemoveLock(m_strName.c_str());
    return false;
  }

  SocketSetBlocking(m_socket, false);

  if (!SetBaudRate(m_iBaudrate))
    return false;

  m_options.c_cflag |= (CLOCAL | CREAD);
  m_options.c_cflag &= ~HUPCL;

  m_options.c_cflag &= ~CSIZE;
  if (m_iDatabits == SERIAL_DATA_BITS_FIVE)  m_options.c_cflag |= CS5;
  if (m_iDatabits == SERIAL_DATA_BITS_SIX)   m_options.c_cflag |= CS6;
  if (m_iDatabits == SERIAL_DATA_BITS_SEVEN) m_options.c_cflag |= CS7;
  if (m_iDatabits == SERIAL_DATA_BITS_EIGHT) m_options.c_cflag |= CS8;

  m_options.c_cflag &= ~PARENB;
  if (m_iParity == SERIAL_PARITY_EVEN || m_iParity == SERIAL_PARITY_ODD)
    m_options.c_cflag |= PARENB;
  if (m_iParity == SERIAL_PARITY_ODD)
    m_options.c_cflag |= PARODD;

#ifdef CRTSCTS
  m_options.c_cflag &= ~CRTSCTS;
#elif defined(CNEW_RTSCTS)
  m_options.c_cflag &= ~CNEW_RTSCTS;
#endif

  if (m_iStopbits == SERIAL_STOP_BITS_ONE) m_options.c_cflag &= ~CSTOPB;
  else m_options.c_cflag |= CSTOPB;
  
  //I guessed a little here
  m_options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | XCASE | ECHOK | ECHONL | ECHOCTL | ECHOPRT | ECHOKE | TOSTOP);

  if (m_iParity == SERIAL_PARITY_NONE)
    m_options.c_iflag &= ~INPCK;
  else
    m_options.c_iflag |= INPCK | ISTRIP;

  m_options.c_iflag &= ~(IXON | IXOFF | IXANY | BRKINT | INLCR | IGNCR | ICRNL | IUCLC | IMAXBEL);
  m_options.c_oflag &= ~(OPOST | ONLCR | OCRNL);

  if (tcsetattr(m_socket, TCSANOW, &m_options) != 0)
  {
    m_strError = strerror(errno);
    RemoveLock(m_strName.c_str());
    return false;
  }
  
  SocketSetBlocking(m_socket, true);
  m_bIsOpen = true;

  return true;
}

bool CSerialSocket::SetBaudRate(uint32_t baudrate)
{
  int rate = IntToBaudrate(baudrate);
  if (rate == -1)
  {
    char buff[255];
    sprintf(buff, "%i is not a valid baudrate", baudrate);
    m_strError = buff;
    return false;
  }

  //get the current port attributes
  if (tcgetattr(m_socket, &m_options) != 0)
  {
    m_strError = strerror(errno);
    return false;
  }

  if (cfsetispeed(&m_options, rate) != 0)
  {
    m_strError = strerror(errno);
    return false;
  }

  if (cfsetospeed(&m_options, rate) != 0)
  {
    m_strError = strerror(errno);
    return false;
  }

  return true;
}
