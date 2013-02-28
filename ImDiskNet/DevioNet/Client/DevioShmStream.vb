﻿Imports LTR.IO.ImDisk.Devio.IMDPROXY_CONSTANTS

Namespace Client

  ''' <summary>
  ''' Derives DevioStream and implements client side of Devio shared memory communication
  ''' proxy.
  ''' </summary>
  Public Class DevioShmStream
    Inherits DevioStream

    Private ReadOnly RequestEvent As EventWaitHandle

    Private ReadOnly ResponseEvent As EventWaitHandle

    Private ReadOnly ServerMutex As Mutex

    Private ReadOnly MapView As SafeBuffer

    ''' <summary>
    ''' Creates a new instance by opening an existing Devio shared memory object and starts
    ''' communication with a Devio service using this shared memory object.
    ''' </summary>
    ''' <param name="name">Name of shared memory object to use for communication.</param>
    ''' <param name="read_only">Specifies if communication should be read-only.</param>
    ''' <returns>Returns new instance of DevioShmStream.</returns>
    Public Shared Function Open(name As String, read_only As Boolean) As DevioShmStream

      Return New DevioShmStream(name, read_only)

    End Function

    ''' <summary>
    ''' Creates a new instance by opening an existing Devio shared memory object and starts
    ''' communication with a Devio service using this shared memory object.
    ''' </summary>
    ''' <param name="name">Name of shared memory object to use for communication.</param>
    ''' <param name="read_only">Specifies if communication should be read-only.</param>
    Public Sub New(name As String, read_only As Boolean)
      MyBase.New(name, read_only)

      Try
        Using Mapping = MemoryMappedFile.OpenExisting(ObjectName,
                                                      MemoryMappedFileRights.ReadWrite)

          MapView = Mapping.CreateViewAccessor().SafeMemoryMappedViewHandle

        End Using

        RequestEvent = New EventWaitHandle(initialState:=False, mode:=EventResetMode.AutoReset, name:="Global\" & ObjectName & "_Request")

        ResponseEvent = New EventWaitHandle(initialState:=False, mode:=EventResetMode.AutoReset, name:="Global\" & ObjectName & "_Response")

        ServerMutex = New Mutex(initiallyOwned:=False, name:="Global\" & ObjectName & "_Server")

        MapView.Write(&H0, IMDPROXY_REQ.IMDPROXY_REQ_INFO)

        RequestEvent.Set()
        If WaitHandle.WaitAny({ResponseEvent, ServerMutex}) <> 0 Then
          Throw New EndOfStreamException("Server exit.")
        End If

        Dim Response = MapView.Read(Of IMDPROXY_INFO_RESP)(&H0)
        Size = CLng(Response.file_size)
        Alignment = CLng(Response.req_alignment)
        Flags = Flags Or Response.flags

      Catch
        Dispose()
        Throw

      End Try

    End Sub

    Public Overrides Sub Close()

      If MapView IsNot Nothing AndAlso RequestEvent IsNot Nothing Then
        Try
          MapView.Write(&H0, IMDPROXY_REQ.IMDPROXY_REQ_CLOSE)
          RequestEvent.Set()

        Catch

        End Try
      End If

      MyBase.Close()

      For Each obj In New IDisposable() {ServerMutex, MapView, RequestEvent, ResponseEvent}
        Try
          If obj IsNot Nothing Then
            obj.Dispose()
          End If

        Catch

        End Try
      Next

    End Sub

    Public Overrides Function Read(buffer As Byte(), offset As Integer, count As Integer) As Integer

      Dim Request As IMDPROXY_READ_REQ
      Request.request_code = IMDPROXY_REQ.IMDPROXY_REQ_READ
      Request.offset = CULng(Position)
      Request.length = CULng(count)

      MapView.Write(&H0, Request)

      RequestEvent.Set()
      If WaitHandle.WaitAny({ResponseEvent, ServerMutex}) <> 0 Then
        Throw New EndOfStreamException("Server exit.")
      End If

      Dim Response = MapView.Read(Of IMDPROXY_READ_RESP)(&H0)
      If Response.errorno <> 0 Then
        Throw New EndOfStreamException("Read error: " & Response.errorno)
      End If
      Dim Length = CInt(Response.length)

      MapView.ReadArray(CULng(IMDPROXY_HEADER_SIZE), buffer, offset, Length)
      Position += Length
      Return Length

    End Function

    Public Overrides Sub Write(buffer As Byte(), offset As Integer, count As Integer)

      Dim Request As IMDPROXY_WRITE_REQ
      Request.request_code = IMDPROXY_REQ.IMDPROXY_REQ_WRITE
      Request.offset = CULng(Position)
      Request.length = CULng(count)

      MapView.Write(&H0, Request)

      MapView.WriteArray(CULng(IMDPROXY_HEADER_SIZE), buffer, offset, count)

      RequestEvent.Set()
      If WaitHandle.WaitAny({ResponseEvent, ServerMutex}) <> 0 Then
        Throw New EndOfStreamException("Server exit.")
      End If

      Dim Response = MapView.Read(Of IMDPROXY_WRITE_RESP)(&H0)
      If Response.errorno <> 0 Then
        Throw New EndOfStreamException("Write error: " & Response.errorno)
      End If
      Dim Length = CInt(Response.length)
      Position += Length

      If Length <> count Then
        Throw New EndOfStreamException("Write length mismatch. Wrote " & Length & " of " & count & " bytes.")
      End If

    End Sub

  End Class

End Namespace
