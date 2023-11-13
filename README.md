# ProcessSpawnControl  
PSC is a command line tool that suspends newly created processes.  
In default mode, suspends subprocesses of the target process, ignoring others.  
In control mode, suspends all newly created processes.  

---  

### Principle of operation  

* drop the detaching loader. It is used to remove the PSC process from the chain  
of parents of the target because the target can check the list of parents;
* run loader. Loader creates a parent process or uses the given ID and  
creates a target process with the specified parent;
* connect to WMI to receive process creation events;
* suspend and resume created processes using NtSuspendProcess and NtResumeProcess;

The PSC searches for parents by comparing the ParentID of the created  
process with the ProcessID of the target process and its subprocesses.  

Situations are possible when the chain of parents is broken and the PSC will not  
be able to establish that the new process belongs to the target one and will not  
suspend it. In this case, you can use the "-ca" key to suspend each created process  
and use your own exclusions list using the "-ex" key to avoid unnecessary triggers.  

The PSC contains a list of exclusions, which contains: "dllhost.exe",  
"searchprotocolhost.exe", "searchfilterhost.exe", "taskhost.exe",  
"conhost.exe", "backgroundtaskhost.exe", "explorer.exe", "msmpeng.exe".  

Exclusions specified using the "-ex" key are added to the  
existing defaults and do not replace them.  

<details>
  <summary>Exclusions example</summary>
  ```
  PSC.exe -tp "path\to\target.exe" -ex "path\to\exclusions.txt"
  ```
  
  ![exclusions](https://github.com/xoreaxecx/ProcessSpawnControl/blob/main/_pics/exclusions.png)    
</details>

---  

### Example  
Run target file with the specified arguments and parent.
```
PSC.exe -tp "C:\tmp\target.exe" -ta "-a arg" -pp "C:\WINDOWS\system32\notepad.exe"
```
<details>
  <summary>Spoiler</summary>
  
  Parent process is running. Target process created and ready to run.
  ![created](https://github.com/xoreaxecx/ProcessSpawnControl/blob/main/_pics/created_console.png)  
  ![created](https://github.com/xoreaxecx/ProcessSpawnControl/blob/main/_pics/created_processes.png)  
  
  Target process is running and has created a subprocess.
  ![suspended](https://github.com/xoreaxecx/ProcessSpawnControl/blob/main/_pics/suspended.png)  
</details>

---  

### Help:  
```
PSC.exe -tp "target.exe" [-ta "-x arg"] [-pp "parent.exe"] [-pa "-x arg"] [-pi PID]
[-ex "exclusions.txt"] [-ca] [-ae] [-nc] [-ao]

-tp : target path. path to the target file.
-ta : target arguments.
-pp : parent path. path to the file to run. used as target's parent.
-pa : parent arguments.
-pi : parent ID. PID of the running process to attach. used as target's parent.
-ex : exclusions. path to the list of exclusions. process names separated by newline.
-ca : control all. suspend all spawned processes except the list of exclusions.
-ae : as explorer. run processes with explorer privileges. as admin if omitted.
-nc : no colors. use only default text color in console output.
-ao : ascii output. replace all unicode chars with "?" in console output.
```

---  

### Notes:
"-ae" key is used to run the target as unelevated (unless you are working under  
an admin account and Explorer.exe is not running with admin privileges).  
"-nc" key is used if the text color is difficult to  
distinguish on the terminal color scheme.  
"-ao" key is used if the file names contain characters that  
cannot be displayed in the terminal with the selected locale.  

---  
