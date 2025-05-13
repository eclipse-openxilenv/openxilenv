from OpenXilEnv.openXilEnv import OpenXilEnv


def main():
    xilEnvInstallationPath = "path/to/xil/installation"
    electricCarIniFilePath = "path/to/ElectricCarSample.ini"

    xil = OpenXilEnv(xilEnvInstallationPath)
    xil.startWithGui(electricCarIniFilePath)

    xil.attachVariables(["FireUp", "PlugIn"])

    xil.writeSignal("FireUp", 1)
    cycles = xil.waitUntilValueMatch("FireUp", 1, 1)
    print(f"Remaining cycles: {cycles}")
    print(f"FireUp: {xil.readSignal('FireUp')}")

    xil.writeSignal("FireUp", 0)
    cycles = xil.waitUntilValueMatch("FireUp", 0, 1)
    print(f"Remaining cycles: {cycles}")
    print(f"FireUp: {xil.readSignal('FireUp')}")

    xil.disconnectAndCloseXil()


if __name__ == "__main__":
    main()
