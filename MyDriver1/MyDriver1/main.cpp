#include <ntifs.h>
#include <ntddk.h>

void MyDriver1Unload(
    [[maybe_unused]] _In_ struct _DRIVER_OBJECT* driverObject)
{
    KdPrint(("MyDriver1.MyDriver1Unload\n"));
}

extern "C" NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT driverObject,
    [[maybe_unused]] _In_ PUNICODE_STRING registryPath)
{
    driverObject->DriverUnload = MyDriver1Unload;
    KdPrint(("MyDriver1.DriverEntry\n"));

    RTL_OSVERSIONINFOW osv{};
    osv.dwOSVersionInfoSize = sizeof(osv);
    NTSTATUS status;

    status = RtlGetVersion(&osv);

    KdPrint(("RtlGetVersion status: %08x\n", status));
    if (NT_SUCCESS(status))
    {
        KdPrint(("OSV: %x, %d.%d.%d\n", osv.dwPlatformId, osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber));
    }

    return STATUS_SUCCESS;
}
