﻿Imports LTR.IO.ImDisk.Devio.IMDPROXY_CONSTANTS
Imports LTR.IO.ImDisk.Devio.Server.Providers

Namespace Server.Services

  ''' <summary>
  ''' Class that implements server end of ImDisk/Devio TCP/IP based communication protocol.
  ''' It uses an object implementing <see>IDevioProvider</see> interface as storage backend
  ''' for I/O requests received from client.
  ''' </summary>
  Public Class DevioTcpService
    Inherits DevioServiceBase

    ''' <summary>
    ''' Server endpoint where this service listens for client connection.
    ''' </summary>
    Public ReadOnly ListenEndPoint As IPEndPoint

    ''' <summary>
    ''' Creates a new service instance with enough data to later run a service that acts as server end in ImDisk/Devio
    ''' TCP/IP based communication.
    ''' </summary>
    ''' <param name="ListenAddress">IP address where service should listen for client connection.</param>
    ''' <param name="ListenPort">IP port where service should listen for client connection.</param>
    ''' <param name="DevioProvider">IDevioProvider object to that serves as storage backend for this service.</param>
    ''' <param name="OwnsProvider">Indicates whether DevioProvider object will be automatically closed when this
    ''' instance is disposed.</param>
    Public Sub New(ListenAddress As IPAddress, ListenPort As Integer, DevioProvider As IDevioProvider, OwnsProvider As Boolean)
      MyBase.New(DevioProvider, OwnsProvider)

      ListenEndPoint = New IPEndPoint(ListenAddress, ListenPort)

    End Sub

    ''' <summary>
    ''' Creates a new service instance with enough data to later run a service that acts as server end in ImDisk/Devio
    ''' TCP/IP based communication.
    ''' </summary>
    ''' <param name="ListenPort">IP port where service should listen for client connection. Instance will listen on all
    ''' interfaces where this port is available.</param>
    ''' <param name="DevioProvider">IDevioProvider object to that serves as storage backend for this service.</param>
    ''' <param name="OwnsProvider">Indicates whether DevioProvider object will be automatically closed when this
    ''' instance is disposed.</param>
    Public Sub New(ListenPort As Integer, DevioProvider As IDevioProvider, OwnsProvider As Boolean)
      MyBase.New(DevioProvider, OwnsProvider)

      ListenEndPoint = New IPEndPoint(IPAddress.Any, ListenPort)

    End Sub

    ''' <summary>
    ''' Runs service that acts as server end in ImDisk/Devio TCP/IP based communication. It will first wait for
    ''' a client to connect, then serve client I/O requests and when client finally requests service to terminate, this
    ''' method returns to caller. To run service in a worker thread that automatically disposes this object after client
    ''' disconnection, call StartServiceThread() instead.
    ''' </summary>
    Public Overrides Sub RunService()

      Try
        Trace.WriteLine("Setting up listener at " & ListenEndPoint.ToString())

        Dim Listener As New TcpListener(ListenEndPoint)

        Try
          Listener.ExclusiveAddressUse = False
          Listener.Start()

        Catch ex As Exception
          Trace.WriteLine("Listen failed: " & ex.ToString())
          OnServiceInitFailed()
          Return

        End Try

        Trace.WriteLine("Raising service ready event.")
        OnServiceReady()

        Dim StopServiceThreadHandler As New Action(AddressOf Listener.Stop)
        AddHandler StopServiceThread, StopServiceThreadHandler
        Dim TcpSocket = Listener.AcceptSocket()
        RemoveHandler StopServiceThread, StopServiceThreadHandler
        Listener.Stop()
        Trace.WriteLine("Connection from " & TcpSocket.RemoteEndPoint.ToString())
        Dim TcpStream As New NetworkStream(TcpSocket, ownsSocket:=True)

        Dim Reader As New BinaryReader(TcpStream, Encoding.Default)
        Dim Writer As New BinaryWriter(New MemoryStream, Encoding.Default)

        Dim ManagedBuffer As Byte() = Nothing

        Do

          Dim RequestCode As IMDPROXY_REQ

          Try
            RequestCode = CType(Reader.ReadUInt64(), IMDPROXY_REQ)

          Catch ex As EndOfStreamException
            Exit Do

          End Try

          'Trace.WriteLine("Got client request: " & RequestCode.ToString())

          Select Case RequestCode

            Case IMDPROXY_REQ.IMDPROXY_REQ_INFO
              SendInfo(Writer)

            Case IMDPROXY_REQ.IMDPROXY_REQ_READ
              ReadData(Reader, Writer, ManagedBuffer)

            Case IMDPROXY_REQ.IMDPROXY_REQ_WRITE
              WriteData(Reader, Writer, ManagedBuffer)

            Case IMDPROXY_REQ.IMDPROXY_REQ_CLOSE
              Trace.WriteLine("Closing connection.")
              Return

            Case Else
              Trace.WriteLine("Unsupported request code: " & RequestCode.ToString())
              Return

          End Select

          'Trace.WriteLine("Sending response and waiting for next request.")

          Writer.Seek(0, SeekOrigin.Begin)
          With DirectCast(Writer.BaseStream, MemoryStream)
            .WriteTo(TcpStream)
            .SetLength(0)
          End With

        Loop

        Trace.WriteLine("Client disconnected.")

      Catch ex As Exception
        Trace.WriteLine("Unhandled exception in service thread: " & ex.ToString())
        OnServiceUnhandledException(New UnhandledExceptionEventArgs(ex, True))

      Finally
        OnServiceShutdown()

      End Try

    End Sub

    Private Sub SendInfo(Writer As BinaryWriter)

      Writer.Write(CULng(DevioProvider.Length))
      Writer.Write(CULng(REQUIRED_ALIGNMENT))
      Writer.Write(CULng(If(DevioProvider.CanWrite, IMDPROXY_FLAGS.IMDPROXY_FLAG_NONE, IMDPROXY_FLAGS.IMDPROXY_FLAG_RO)))

    End Sub

    Private Sub ReadData(Reader As BinaryReader, Writer As BinaryWriter, Data As Byte())

      Dim Offset = Reader.ReadInt64()
      Dim ReadLength = CInt(Reader.ReadUInt64())
      If Data Is Nothing OrElse Data.Length < ReadLength Then
        Array.Resize(Data, ReadLength)
      End If
      Dim WriteLength As ULong
      Dim ErrorCode As ULong

      Try
        WriteLength = CULng(DevioProvider.Read(Data, 0, ReadLength, Offset))
        ErrorCode = 0

      Catch ex As Exception
        Trace.WriteLine(ex.ToString())
        Trace.WriteLine("Read request at " & Offset.ToString("X").PadLeft(8, "0"c) & " for " & ReadLength & " bytes.")
        ErrorCode = 1
        WriteLength = 0

      End Try

      Writer.Write(ErrorCode)
      Writer.Write(WriteLength)
      If WriteLength > 0 Then
        Writer.Write(Data, 0, CInt(WriteLength))
      End If

    End Sub

    Private Sub WriteData(Reader As BinaryReader, Writer As BinaryWriter, Data As Byte())

      Dim Offset = Reader.ReadInt64()
      Dim Length = Reader.ReadUInt64()
      If Data Is Nothing OrElse Data.Length < Length Then
        Array.Resize(Data, CInt(Length))
      End If

      Dim ReadLength = Reader.Read(Data, 0, CInt(Length))
      Dim WriteLength As ULong
      Dim ErrorCode As ULong

      Try
        WriteLength = CULng(DevioProvider.Write(Data, 0, ReadLength, Offset))
        ErrorCode = 0

      Catch ex As Exception
        Trace.WriteLine(ex.ToString())
        Trace.WriteLine("Write request at " & Offset.ToString("X").PadLeft(8, "0"c) & " for " & Length & " bytes.")
        ErrorCode = 1
        WriteLength = 0

      End Try

      Writer.Write(ErrorCode)
      Writer.Write(WriteLength)

    End Sub

    Protected Overrides ReadOnly Property ImDiskProxyObjectName As String
      Get
        Dim EndPoint = ListenEndPoint
        If EndPoint.Address.Equals(IPAddress.Any) Then
          EndPoint = New IPEndPoint(IPAddress.Loopback, EndPoint.Port)
        End If
        Return EndPoint.ToString()
      End Get
    End Property

    Protected Overrides ReadOnly Property ImDiskProxyModeFlags As ImDiskFlags
      Get
        Return ImDiskFlags.TypeProxy Or ImDiskFlags.ProxyTypeTCP
      End Get
    End Property

  End Class

End Namespace
