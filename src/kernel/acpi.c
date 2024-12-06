#include "acpi.h"
#include "io.h"
#include <string.h>

// ACPI Table Signatures
#define ACPI_FACP_SIG "FACP"
#define ACPI_DSDT_SIG "DSDT"

// ACPI PM1 Control Register Bits
#define ACPI_PM1_SLP_TYP_OFFSET 10
#define ACPI_PM1_SLP_EN (1 << 13)

// ACPI Tables
static rsdp_descriptor_t* rsdp = NULL;
static acpi_header_t* rsdt = NULL;
static fadt_t* facp = NULL;
static acpi_header_t* dsdt = NULL;

// ACPI PM1 Control Registers
static uint32_t pm1a_control = 0;
static uint32_t pm1b_control = 0;

// Find RSDP in memory
static rsdp_descriptor_t* find_rsdp(void) {
    uint8_t* addr;
    
    // Search in EBDA first (first 1KB)
    uint32_t ebda = *((uint16_t*)0x40E) << 4;
    for (addr = (uint8_t*)ebda; addr < (uint8_t*)(ebda + 1024); addr++) {
        if (memcmp(addr, "RSD PTR ", 8) == 0) {
            return (rsdp_descriptor_t*)addr;
        }
    }
    
    // Search in BIOS area (0xE0000 - 0xFFFFF)
    for (addr = (uint8_t*)0xE0000; addr < (uint8_t*)0x100000; addr++) {
        if (memcmp(addr, "RSD PTR ", 8) == 0) {
            return (rsdp_descriptor_t*)addr;
        }
    }
    
    return NULL;
}

// Initialize ACPI subsystem
void acpi_init(void) {
    // Find RSDP
    rsdp = find_rsdp();
    if (!rsdp) {
        return;
    }
    
    // Get RSDT
    rsdt = (acpi_header_t*)(uint32_t)rsdp->rsdt_address;
    if (!rsdt || memcmp(rsdt->signature, "RSDT", 4) != 0) {
        return;
    }
    
    // Find FACP (FADT)
    uint32_t entries = (rsdt->length - sizeof(acpi_header_t)) / 4;
    uint32_t* table_ptrs = (uint32_t*)(rsdt + 1);
    
    for (uint32_t i = 0; i < entries; i++) {
        acpi_header_t* header = (acpi_header_t*)(uint32_t)table_ptrs[i];
        if (memcmp(header->signature, ACPI_FACP_SIG, 4) == 0) {
            facp = (fadt_t*)header;
            break;
        }
    }
    
    if (!facp) {
        return;
    }
    
    // Get PM1 Control registers from FACP
    pm1a_control = facp->pm1a_control_block;
    pm1b_control = facp->pm1b_control_block;
}

// Shutdown the system using ACPI
void acpi_shutdown(void) {
    if (!facp) {
        return;
    }
    
    // Set SLP_TYP and SLP_EN bits in PM1a control register
    if (pm1a_control) {
        outw(pm1a_control, (1 << 13) | (5 << 10));
    }
    
    // Set SLP_TYP and SLP_EN bits in PM1b control register if it exists
    if (pm1b_control) {
        outw(pm1b_control, (1 << 13) | (5 << 10));
    }
}
