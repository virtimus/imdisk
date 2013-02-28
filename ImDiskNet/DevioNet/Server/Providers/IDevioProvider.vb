﻿Namespace Server.Providers

  ''' <summary>
  ''' <para>Interface with functionality required for a class to represent an object that can
  ''' service ImDisk/Devio I/O requests at server side. Classes implementing this
  ''' interface provides functions for determining properties such as virtual disk size
  ''' and whether virtual disk is writable, as well as functions for reading from and
  ''' optionally writing to virtual disk.</para>
  ''' 
  ''' <para>To make implementation easier, two base classes implement this interface. That is
  ''' DevioProviderManagedBase and DevioProviderUnmanagedBase. The first one makes it
  ''' possible to implement this interface by just overriding a few functions for reading
  ''' data to and writing data from managed byte arrays. The latter one makes it possible
  ''' to implement this interface by just overriding a few functions for reading data to
  ''' and writing data from unmanaged memory provided a pointer to unmanaged block of
  ''' memory.</para>
  ''' </summary>
  Public Interface IDevioProvider
    Inherits IDisposable

    ''' <summary>
    ''' Size of virtual disk.
    ''' </summary>
    ''' <value>Size of virtual disk.</value>
    ''' <returns>Size of virtual disk.</returns>
    ReadOnly Property Length As Long

    ''' <summary>
    ''' Sector size of virtual disk.
    ''' </summary>
    ''' <value>Sector size of virtual disk.</value>
    ''' <returns>Sector size of virtual disk.</returns>
    ReadOnly Property SectorSize As UInteger

    ''' <summary>
    ''' Determines whether virtual disk is writable or read-only.
    ''' </summary>
    ''' <value>True if virtual disk can be written to through this instance, or False
    ''' if it is opened for reading only.</value>
    ''' <returns>True if virtual disk can be written to through this instance, or False
    ''' if it is opened for reading only.</returns>
    ReadOnly Property CanWrite As Boolean

    ''' <summary>
    ''' Reads bytes from virtual disk to a memory area specified by a pointer to unmanaged memory.
    ''' </summary>
    ''' <param name="buffer">Pointer to unmanaged memory where read bytes are stored.</param>
    ''' <param name="bufferoffset">Offset in unmanaged memory buffer where bytes are stored.</param>
    ''' <param name="count">Number of bytes to read from virtual disk device.</param>
    ''' <param name="fileoffset">Offset at virtual disk device where read starts.</param>
    ''' <returns>Returns number of bytes read from device that were stored at specified memory position.</returns>
    Function Read(buffer As IntPtr, bufferoffset As Integer, count As Integer, fileoffset As Long) As Integer

    ''' <summary>
    ''' Writes out bytes to virtual disk device from a memory area specified by a pointer to unmanaged memory.
    ''' </summary>
    ''' <param name="buffer">Pointer to unmanaged memory area containing bytes to write out to device.</param>
    ''' <param name="bufferoffset">Offset in unmanaged memory buffer where bytes to write are located.</param>
    ''' <param name="count">Number of bytes to write to virtual disk device.</param>
    ''' <param name="fileoffset">Offset at virtual disk device where write starts.</param>
    ''' <returns>Returns number of bytes written to device.</returns>
    Function Write(buffer As IntPtr, bufferoffset As Integer, count As Integer, fileoffset As Long) As Integer

    ''' <summary>
    ''' Reads bytes from virtual disk to a byte array.
    ''' </summary>
    ''' <param name="buffer">Byte array with enough size where read bytes are stored.</param>
    ''' <param name="bufferoffset">Offset in array where bytes are stored.</param>
    ''' <param name="count">Number of bytes to read from virtual disk device.</param>
    ''' <param name="fileoffset">Offset at virtual disk device where read starts.</param>
    ''' <returns>Returns number of bytes read from device that were stored in byte array.</returns>
    Function Read(buffer As Byte(), bufferoffset As Integer, count As Integer, fileoffset As Long) As Integer

    ''' <summary>
    ''' Writes out bytes from byte array to virtual disk device.
    ''' </summary>
    ''' <param name="buffer">Byte array containing bytes to write out to device.</param>
    ''' <param name="bufferoffset">Offset in array where bytes to write start.</param>
    ''' <param name="count">Number of bytes to write to virtual disk device.</param>
    ''' <param name="fileoffset">Offset at virtual disk device where write starts.</param>
    ''' <returns>Returns number of bytes written to device.</returns>
    Function Write(buffer As Byte(), bufferoffset As Integer, count As Integer, fileoffset As Long) As Integer

  End Interface

End Namespace
