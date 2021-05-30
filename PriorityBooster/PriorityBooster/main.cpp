#include <ntifs.h>

#include <ntddk.h>

#include "PriorityBooster.h"

void PriorityBoosterUnload(_In_ struct _DRIVER_OBJECT* DriverObject);
NTSTATUS PriorityBoosterCreate(_In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp);
NTSTATUS PriorityBoosterClose(_In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp);
NTSTATUS PriorityBoosterDeviceControl(_In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp);

const UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(LR"(\Device\PriorityBooster)");
const UNICODE_STRING DeviceSymLink = RTL_CONSTANT_STRING(LR"(\??\PriorityBooster)");

extern "C" NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    [[maybe_unused]] _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;

    DriverObject->DriverUnload = PriorityBoosterUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = PriorityBoosterCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = PriorityBoosterClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PriorityBoosterDeviceControl;

    PDEVICE_OBJECT deviceObject;
    status = IoCreateDevice(
        DriverObject,
        0,
        const_cast<PUNICODE_STRING>(&DeviceName),
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &deviceObject);

    if (!NT_SUCCESS(status))
    {
        KdPrint(("[PriorityBooster] IoCreateDevice failed (%08x)\n", status));
        return status;
    }

    status = IoCreateSymbolicLink(
        const_cast<PUNICODE_STRING>(&DeviceSymLink),
        const_cast<PUNICODE_STRING>(&DeviceName));

    if (!NT_SUCCESS(status))
    {
        KdPrint(("[PriorityBooster] IoCreateSymbolicLink failed (%08x)\n", status));
        IoDeleteDevice(deviceObject);
        return status;
    }

    return STATUS_SUCCESS;
}

_Use_decl_annotations_ void
PriorityBoosterUnload(
    _In_ struct _DRIVER_OBJECT* driverObject)
{
    NTSTATUS status;

    status = IoDeleteSymbolicLink(const_cast<PUNICODE_STRING>(&DeviceSymLink));
    ASSERT(NT_SUCCESS(status));

    IoDeleteDevice(driverObject->DeviceObject);

    KdPrint(("[PriorityBooster] Unloaded\n"));
}

_Use_decl_annotations_
    NTSTATUS
    PriorityBoosterCreate(
        [[maybe_unused]] _In_ struct _DEVICE_OBJECT* DeviceObject,
        _Inout_ struct _IRP* Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
    NTSTATUS
    PriorityBoosterClose(
        [[maybe_unused]] _In_ struct _DEVICE_OBJECT* DeviceObject,
        _Inout_ struct _IRP* Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
    NTSTATUS
    PriorityBoosterDeviceControl(
        [[maybe_unused]] _In_ struct _DEVICE_OBJECT* DeviceObject,
        _Inout_ struct _IRP* Irp)
{
    NTSTATUS status;
    auto stack = IoGetCurrentIrpStackLocation(Irp);

    switch (stack->Parameters.DeviceIoControl.IoControlCode)
    {
    case PRIORITY_BOOSTER_IOCTL_SET_THREAD_PRIORITY:
    {
        if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SetThreadPriorityData))
        {
            KdPrint(("[PriorityBooster] Invalid InputBufferLength %d\n", stack->Parameters.DeviceIoControl.InputBufferLength));
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        const auto* stpd = reinterpret_cast<SetThreadPriorityData*>(stack->Parameters.DeviceIoControl.Type3InputBuffer);
        if (stpd == nullptr)
        {
            KdPrint(("[PriorityBooster] Invalid Type3InputBuffer %p\n", stpd));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        const HANDLE hThread = stpd->Thread;
        const int priority = stpd->Priority;

        if (hThread == nullptr)
        {
            KdPrint(("[PriorityBooster] Invalid Thread %p\n", hThread));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        if (priority < 1 || 31 < priority)
        {
            KdPrint(("[PriorityBooster] Invalid Priority %d\n", priority));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        PKTHREAD thread;
        status = ObReferenceObjectByHandle(hThread, THREAD_SET_INFORMATION, *PsThreadType, UserMode, reinterpret_cast<PVOID*>(&thread), nullptr);
        if (!NT_SUCCESS(status))
        {
            KdPrint(("[PriorityBooster] Invalid Thread %p (%08x)\n", hThread, status));
            break;
        }

        status = KeSetPriorityThread(thread, priority);
        ObDereferenceObject(thread);
        if (!NT_SUCCESS(status))
        {
            KdPrint(("[PriorityBooster] KeSetPriorityThread failed %p, %d (%08x)\n", thread, priority, status));
            break;
        }

        KdPrint(("[PriorityBooster] KeSetPriorityThread successful %p, %d (%08x)\n", thread, priority, status));
        break;
    }

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}
