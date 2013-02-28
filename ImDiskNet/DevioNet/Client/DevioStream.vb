﻿Namespace Client

  ''' <summary>
  ''' Base class for classes that implement Stream for client side of ImDisk/Devio protocol.
  ''' </summary>
  Public MustInherit Class DevioStream
    Inherits Stream

    ''' <summary>
    ''' Object name used by proxy implementation.
    ''' </summary>
    Public ReadOnly ObjectName As String

    ''' <summary>
    ''' Virtual disk size of server object.
    ''' </summary>
    Protected Size As Long

    ''' <summary>
    ''' Alignment requirement for I/O at server.
    ''' </summary>
    Protected Alignment As Long

    ''' <summary>
    ''' ImDisk proxy flags specified for proxy connection.
    ''' </summary>
    Protected Flags As IMDPROXY_FLAGS

    ''' <summary>
    ''' Initiates a new instance with supplied object name and read-only flag.
    ''' </summary>
    ''' <param name="name">Object name used by proxy implementation.</param>
    ''' <param name="read_only">Flag set to true to indicate read-only proxy
    ''' operation.</param>
    Protected Sub New(name As String, read_only As Boolean)
      ObjectName = name
      If read_only Then
        Flags = IMDPROXY_FLAGS.IMDPROXY_FLAG_RO
      End If
    End Sub

    ''' <summary>
    ''' Indicates whether Stream is readable. This implementation returns a
    ''' constant value of True, because ImDisk/Devio proxy implementations are
    ''' always readable.
    ''' </summary>
    Public Overrides ReadOnly Property CanRead As Boolean
      Get
        Return True
      End Get
    End Property

    ''' <summary>
    ''' Indicates whether Stream is seekable. This implementation returns a
    ''' constant value of True.
    ''' </summary>
    Public Overrides ReadOnly Property CanSeek As Boolean
      Get
        Return True
      End Get
    End Property

    ''' <summary>
    ''' Indicates whether Stream is writable. This implementation returns True
    ''' unless ProxyFlags property contains IMDPROXY_FLAGS.IMDPROXY_FLAG_RO value.
    ''' </summary>
    Public Overrides ReadOnly Property CanWrite As Boolean
      Get
        Return (Flags And IMDPROXY_FLAGS.IMDPROXY_FLAG_RO) = 0
      End Get
    End Property

    ''' <summary>
    ''' This implementation does not do anything.
    ''' </summary>
    Public Overrides Sub Flush()

    End Sub

    ''' <summary>
    ''' When overriden in a derived class, closes communication and causes server side to exit.
    ''' </summary>
    Public Overrides Sub Close()
      MyBase.Close()
    End Sub

    ''' <summary>
    ''' Returns current virtual disk size.
    ''' </summary>
    Public Overrides ReadOnly Property Length As Long
      Get
        Return Size
      End Get
    End Property

    ''' <summary>
    ''' Current byte position in Stream.
    ''' </summary>
    Public Overrides Property Position As Long

    ''' <summary>
    ''' Moves current position in Stream.
    ''' </summary>
    ''' <param name="offset">Byte offset to move. Can be negative to move backwards.</param>
    ''' <param name="origin">Origin from where number of bytes to move counts.</param>
    ''' <returns>Returns new absolute position in Stream.</returns>
    Public Overrides Function Seek(offset As Long, origin As SeekOrigin) As Long

      Select Case origin

        Case SeekOrigin.Begin
          Position = offset

        Case SeekOrigin.Current
          Position += offset

        Case SeekOrigin.End
          Position = Size + offset

        Case Else
          Throw New ArgumentException("Invalid origin", "origin")

      End Select

      Return Position

    End Function

    ''' <summary>
    ''' This method is not supported in this implementation and throws a NotImplementedException.
    ''' A derived class can override this method to implement a resize feature.
    ''' </summary>
    ''' <param name="value">New total size of Stream</param>
    Public Overrides Sub SetLength(value As Long)
      Throw New NotImplementedException("SetLength() not implemented for DevioStream objects.")
    End Sub

    ''' <summary>
    ''' Alignment requirement for I/O at server.
    ''' </summary>
    Public ReadOnly Property RequiredAlignment As Long
      Get
        Return Alignment
      End Get
    End Property

    ''' <summary>
    ''' ImDisk proxy flags specified for proxy connection.
    ''' </summary>
    Public ReadOnly Property ProxyFlags As IMDPROXY_FLAGS
      Get
        Return Flags
      End Get
    End Property

  End Class

End Namespace
