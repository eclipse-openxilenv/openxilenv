# OpenXilEnv

The Python package **OpenXilEnv** provides an easy way to interact with a XIL environment in automation scripts.


## Build and installation
The following PIP packages are mandatory. So, make sure to install them:
- `setuptools`
- `wheel`

### Build the package
To build the package, navigate to the directory containing the `setup.py` file and run the following command:

```bash
python setup.py bdist_wheel
```

This command will generate a `build` and `dist` folder. Inside the `dist` folder you will find the wheel package to install.

### Install the package

To install the package in your Python environment, run this command:

```bash
pip install path/to/dist/OpenXilEnv-0.1-py3-none-any.whl
```

The version of the package might differ. Be carefully here and put the correct path and name of the wheel package inside the command.

<br>

---

<br>

## Example: `automationExample.py`

The script `automationExample.py` demonstrates how to use the **OpenXilEnv** package. Keep in mind that you must modify the variables `xilEnvInstallationPath` and `electricCarIniFilePath` to their correct paths in your case. <br>
This example uses the electric car example. Make sure to configure the OpenXilEnv application correctly with CMake, so that the example are also built.

### Workflow:
1. The XIL environment is started with the GUI.
2. Variables like `FireUp` and `PlugIn` are attached.
3. Signals are written and read, e.g., activating (`FireUp = 1`) and deactivating (`FireUp = 0`) a signal.
4. The script waits for signal changes before printing the remaining cycles.
5. Finally, the connection to the XIL environment is disconnected, and the environment is closed.

### Example Output:
```plaintext
Remaining cycles: 98
FireUp: 1
Remaining cycles: 98
FireUp: 0
```