﻿Imports LTR.IO.ImDisk.Devio.IMDPROXY_CONSTANTS
Imports LTR.IO.ImDisk.Devio.Server.Providers

Namespace Server.Services

  ''' <summary>
  ''' Class that implements server end of ImDisk/Devio shared memory based communication
  ''' protocol. It uses an object implementing <see>IDevioProvider</see> interface as
  ''' storage backend for I/O requests received from client.
  ''' </summary>
  Public Class DevioShmService
    Inherits DevioServiceBase

    ''' <summary>
    ''' Object name of shared memory file mapping object created by this instance.
    ''' </summary>
    Public ReadOnly ObjectName As String

    ''' <summary>
    ''' Buffer size used by this instance.
    ''' </summary>
    Public ReadOnly BufferSize As Long

    ''' <summary>
    ''' Buffer size that will be automatically selected on this platform when
    ''' an instance is created by a constructor without a BufferSize argument.
    ''' </summary>
    Public Shared ReadOnly Property DefaultBufferSize As Long
      Get
        If Environment.OSVersion.Version.Major > 5 Then
          Return 2097152 + IMDPROXY_HEADER_SIZE
        Else
          Return 1048576 + IMDPROXY_HEADER_SIZE
        End If
      End Get
    End Property

    Private Shared _random As New Random
    Private Shared Function GetNextRandomValue() As Integer
      SyncLock _random
        Return _random.Next()
      End SyncLock
    End Function

    ''' <summary>
    ''' Creates a new service instance with enough data to later run a service that acts as server end in ImDisk/Devio
    ''' shared memory based communication.
    ''' </summary>
    ''' <param name="ObjectName">Object name of shared memory file mapping object created by this instance.</param>
    ''' <param name="DevioProvider">IDevioProvider object to that serves as storage backend for this service.</param>
    ''' <param name="OwnsProvider">Indicates whether DevioProvider object will be automatically closed when this
    ''' instance is disposed.</param>
    ''' <param name="BufferSize">Buffer size to use for shared memory I/O communication.</param>
    Public Sub New(ObjectName As String, DevioProvider As IDevioProvider, OwnsProvider As Boolean, BufferSize As Long)
      MyBase.New(DevioProvider, OwnsProvider)

      Me.ObjectName = ObjectName
      Me.BufferSize = BufferSize

    End Sub

    ''' <summary>
    ''' Creates a new service instance with enough data to later run a service that acts as server end in ImDisk/Devio
    ''' shared memory based communication. A default buffer size will be used.
    ''' </summary>
    ''' <param name="ObjectName">Object name of shared memory file mapping object created by this instance.</param>
    ''' <param name="DevioProvider">IDevioProvider object to that serves as storage backend for this service.</param>
    ''' <param name="OwnsProvider">Indicates whether DevioProvider object will be automatically closed when this
    ''' instance is disposed.</param>
    Public Sub New(ObjectName As String, DevioProvider As IDevioProvider, OwnsProvider As Boolean)
      MyClass.New(ObjectName, DevioProvider, OwnsProvider, DefaultBufferSize)
    End Sub

    ''' <summary>
    ''' Creates a new service instance with enough data to later run a service that acts as server end in ImDisk/Devio
    ''' shared memory based communication. A default buffer size and a random object name will be used.
    ''' </summary>
    ''' <param name="DevioProvider">IDevioProvider object to that serves as storage backend for this service.</param>
    ''' <param name="OwnsProvider">Indicates whether DevioProvider object will be automatically closed when this
    ''' instance is disposed.</param>
    Public Sub New(DevioProvider As IDevioProvider, OwnsProvider As Boolean)
      MyClass.New(DevioProvider, OwnsProvider, DefaultBufferSize)
    End Sub

    ''' <summary>
    ''' Creates a new service instance with enough data to later run a service that acts as server end in ImDisk/Devio
    ''' shared memory based communication. A random object name will be used.
    ''' </summary>
    ''' <param name="DevioProvider">IDevioProvider object to that serves as storage backend for this service.</param>
    ''' <param name="OwnsProvider">Indicates whether DevioProvider object will be automatically closed when this
    ''' instance is disposed.</param>
    ''' <param name="BufferSize">Buffer size to use for shared memory I/O communication.</param>
    Public Sub New(DevioProvider As IDevioProvider, OwnsProvider As Boolean, BufferSize As Long)
      MyClass.New("devio-" & GetNextRandomValue(), DevioProvider, OwnsProvider, BufferSize)
    End Sub

    ''' <summary>
    ''' Runs service that acts as server end in ImDisk/Devio shared memory based communication. It will first wait for
    ''' a client to connect, then serve client I/O requests and when client finally requests service to terminate, this
    ''' method returns to caller. To run service in a worker thread that automatically disposes this object after client
    ''' disconnection, call StartServiceThread() instead.
    ''' </summary>
    Public Overrides Sub RunService()

      Try
        Trace.WriteLine("Creating objects for shared memory communication '" & ObjectName & "'.")

        Dim RequestEvent As New EventWaitHandle(initialState:=False, mode:=EventResetMode.AutoReset, name:="Global\" & ObjectName & "_Request")

        Dim ResponseEvent As New EventWaitHandle(initialState:=False, mode:=EventResetMode.AutoReset, name:="Global\" & ObjectName & "_Response")

        Dim ServerMutex As New Mutex(initiallyOwned:=False, name:="Global\" & ObjectName & "_Server")

        If ServerMutex.WaitOne(0) = False Then
          Trace.WriteLine("Service busy.")
          OnServiceInitFailed()
          Return
        End If

        Dim Mapping = MemoryMappedFile.CreateNew("Global\" & ObjectName,
                                                 BufferSize,
                                                 MemoryMappedFileAccess.ReadWrite,
                                                 MemoryMappedFileOptions.None,
                                                 Nothing,
                                                 HandleInheritability.None)

        Dim MapView = Mapping.CreateViewAccessor().SafeMemoryMappedViewHandle

        Trace.WriteLine("Created shared memory object, " & MapView.ByteLength & " bytes.")

        Trace.WriteLine("Raising service ready event.")
        OnServiceReady()

        Trace.WriteLine("Waiting for client to connect.")

        Using StopServiceThreadEvent As New EventWaitHandle(initialState:=False, mode:=EventResetMode.ManualReset)
          Dim StopServiceThreadHandler As New Action(AddressOf StopServiceThreadEvent.Set)
          AddHandler StopServiceThread, StopServiceThreadHandler
          Dim WaitEvents = {RequestEvent, StopServiceThreadEvent}
          Dim EventIndex = WaitHandle.WaitAny(WaitEvents)
          RemoveHandler StopServiceThread, StopServiceThreadHandler

          Trace.WriteLine("Wait finished. Disposing file mapping object.")

          Mapping.Dispose()
          Mapping = Nothing

          If WaitEvents(EventIndex) Is StopServiceThreadEvent Then
            Trace.WriteLine("Service thread exit request.")
            Return
          End If
        End Using

        Trace.WriteLine("Client connected, waiting for request.")

        Using MapView

          Do
            Dim RequestCode = MapView.Read(Of IMDPROXY_REQ)(&H0)

            'Trace.WriteLine("Got client request: " & RequestCode.ToString())

            Select Case RequestCode

              Case IMDPROXY_REQ.IMDPROXY_REQ_INFO
                SendInfo(MapView)

              Case IMDPROXY_REQ.IMDPROXY_REQ_READ
                ReadData(MapView)

              Case IMDPROXY_REQ.IMDPROXY_REQ_WRITE
                WriteData(MapView)

              Case IMDPROXY_REQ.IMDPROXY_REQ_CLOSE
                Trace.WriteLine("Closing connection.")
                Return

              Case Else
                Trace.WriteLine("Unsupported request code: " & RequestCode.ToString())
                Return

            End Select

            'Trace.WriteLine("Sending response and waiting for next request.")

            If WaitHandle.SignalAndWait(ResponseEvent, RequestEvent) = False Then
              Trace.WriteLine("Synchronization failed.")
            End If

          Loop

        End Using

        Trace.WriteLine("Client disconnected.")

      Catch ex As Exception
        Trace.WriteLine("Unhandled exception in service thread: " & ex.ToString())
        OnServiceUnhandledException(New UnhandledExceptionEventArgs(ex, True))

      Finally
        OnServiceShutdown()

      End Try

    End Sub

    Private Sub SendInfo(MapView As SafeBuffer)

      Dim Info As IMDPROXY_INFO_RESP
      Info.file_size = CULng(DevioProvider.Length)
      Info.req_alignment = CULng(REQUIRED_ALIGNMENT)
      Info.flags = If(DevioProvider.CanWrite, IMDPROXY_FLAGS.IMDPROXY_FLAG_NONE, IMDPROXY_FLAGS.IMDPROXY_FLAG_RO)

      MapView.Write(&H0, Info)

    End Sub

    Private Sub ReadData(MapView As SafeBuffer)

      Dim Request = MapView.Read(Of IMDPROXY_READ_REQ)(&H0)

      Dim Offset = CLng(Request.offset)
      Dim ReadLength = CInt(Request.length)

      Static largest_request As Integer
      If ReadLength > largest_request Then
        largest_request = ReadLength
        Trace.WriteLine("Largest requested read size is now: " & largest_request & " bytes")
      End If

      Dim Response As IMDPROXY_READ_RESP

      Try
        If ReadLength > MapView.ByteLength - IMDPROXY_HEADER_SIZE Then
          Trace.WriteLine("Requested read length " & ReadLength & ", lowered to " & CInt(MapView.ByteLength - CInt(IMDPROXY_HEADER_SIZE)) & " bytes.")
          ReadLength = CInt(MapView.ByteLength - CInt(IMDPROXY_HEADER_SIZE))
        End If
        Response.length = CULng(DevioProvider.Read(MapView.DangerousGetHandle(), IMDPROXY_HEADER_SIZE, ReadLength, Offset))
        Response.errorno = 0

      Catch ex As Exception
        Trace.WriteLine(ex.ToString())
        Trace.WriteLine("Read request at " & Offset.ToString("X").PadLeft(8, "0"c) & " for " & ReadLength & " bytes.")
        Response.errorno = 1
        Response.length = 0

      End Try

      MapView.Write(&H0, Response)

    End Sub

    Private Sub WriteData(MapView As SafeBuffer)

      Dim Request = MapView.Read(Of IMDPROXY_WRITE_REQ)(&H0)

      Dim Offset = CLng(Request.offset)
      Dim Length = CInt(Request.length)

      Static largest_request As Integer
      If Length > largest_request Then
        largest_request = Length
        Trace.WriteLine("Largest requested write size is now: " & largest_request & " bytes")
      End If

      Dim Response As IMDPROXY_WRITE_RESP

      Try
        If Length > MapView.ByteLength - IMDPROXY_HEADER_SIZE Then
          Throw New Exception("Requested write length " & Length & ". Buffer size is " & CInt(MapView.ByteLength - CInt(IMDPROXY_HEADER_SIZE)) & " bytes.")
        End If
        Length = DevioProvider.Write(MapView.DangerousGetHandle(), IMDPROXY_HEADER_SIZE, Length, Offset)
        Response.errorno = 0
        Response.length = CULng(Length)

      Catch ex As Exception
        Trace.WriteLine(ex.ToString())
        Trace.WriteLine("Write request at " & Offset.ToString("X").PadLeft(8, "0"c) & " for " & Length & " bytes.")
        Response.errorno = 1
        Response.length = 0

      End Try

      MapView.Write(&H0, Response)

    End Sub

    Protected Overrides ReadOnly Property ImDiskProxyObjectName As String
      Get
        Return ObjectName
      End Get
    End Property

    Protected Overrides ReadOnly Property ImDiskProxyModeFlags As ImDiskFlags
      Get
        Return ImDiskFlags.TypeProxy Or ImDiskFlags.ProxyTypeSharedMemory
      End Get
    End Property

  End Class

End Namespace
