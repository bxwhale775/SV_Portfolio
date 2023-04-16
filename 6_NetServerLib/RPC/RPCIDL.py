import json
import sys
import re
from pathlib import Path


def CheckArrayExpression(s):
    match = re.findall(r'\[(\d+)\]', s)
    if match:
        return [int(i) for i in match]
    else:
        return None

def ExtractArrayName(s):
    arrname = s.split('[')[0]
    return arrname

def BuildProxy(IDLdata):
    proxy_str = ''
    RPCIDcnt = 0
    
    #Write proxy code
    proxy_str += '#pragma once\n\n'
    rpcid_type = IDLdata['settings']['RPCID_type']
    proxy_str += f"using RPCID = {rpcid_type};\n"

    proxy_str += 'class CNetServer;\n\n'
    proxy_str += f"class {IDLdata['settings']['proxy_class_name']} : public SuperRPCProxy {{\n"
    
    proxy_str += 'public:\n'
    for k, v in IDLdata['RPCs'].items():
        isValuesExist = True
        values = None
        try:
            values = v['values']
        except KeyError:
            isValuesExist = False
        rpcid = 0
        try:
            rpcid = v['RPCID']
            RPCIDcnt = rpcid
        except KeyError:
            rpcid = RPCIDcnt
        RPCIDcnt += 1

        proxy_str += f'	bool {k}(NID __RPC__nid'
        if isValuesExist:
            for vk, vv in values.items():
                arr_sizes = CheckArrayExpression(vk)
                if arr_sizes is None:
                    proxy_str += f', {vv} {vk}'
                else:
                    proxy_str += f', {vv} (&{ExtractArrayName(vk)})'
                    for arrsiz in arr_sizes:
                        proxy_str += f'[{arrsiz}]'

        proxy_str += ') {\n'
        proxy_str += '\t\tbool __RPC__ret;\n'
        proxy_str += '\t\tSerialBuffer* __RPC__buf = __RPC__sv->AllocPacket();\n'
        proxy_str += '\t\tSerialBufferIO __RPC__bufio(__RPC__buf);\n'
        proxy_str += f'\t\t__RPC__bufio << (RPCID){rpcid};\n'
        if isValuesExist:
            for vk, vv in values.items():
                arr_sizes = CheckArrayExpression(vk)
                if arr_sizes is None:
                    proxy_str += f'\t\t__RPC__bufio << {vk};\n'
                else:
                    tab_cnt = 2
                    arr_sizes_num = len(arr_sizes)
                    for idx in range(0,arr_sizes_num):
                        for _ in range(0, tab_cnt):
                            proxy_str += '\t'
                        rpc_idx_str = f'__RPC__idx_{idx}'
                        proxy_str += f"for(int {rpc_idx_str}=0; {rpc_idx_str}<{arr_sizes[idx]}; {rpc_idx_str}++) {{\n"
                        tab_cnt += 1
                    for _ in range(0, tab_cnt):
                        proxy_str += '\t'
                    proxy_str += f"__RPC__bufio << {ExtractArrayName(vk)}"
                    for idx in range(0,arr_sizes_num):
                        rpc_idx_str = f'__RPC__idx_{idx}'
                        proxy_str += f"[{rpc_idx_str}]"
                    proxy_str += ';\n'
                    for idx in range(0,arr_sizes_num):
                        tab_cnt -= 1
                        for _ in range(0, tab_cnt):
                            proxy_str += '\t'
                        proxy_str += '}\n'
        proxy_str += '\t\t__RPC__ret = __RPC__sv->SendPacket(__RPC__nid, __RPC__buf);\n'
        proxy_str += '\t\t__RPC__sv->FreePacket(__RPC__buf);\n'
        proxy_str += '\t\treturn __RPC__ret;\n'
        proxy_str += '\t}\n'

    proxy_str += '};\n'

    return proxy_str

def BuildStub(IDLdata):
    stub_str = ''
    RPCIDcnt = 0

    #Write stub code
    stub_str += '#pragma once\n'
    stub_str += '#include <functional>\n\n'

    rpcid_type = IDLdata['settings']['RPCID_type']
    stub_str += f"using RPCID = {rpcid_type};\n"

    stub_str += 'class CNetServer;\n\n'
    stub_str += f"class {IDLdata['settings']['stub_class_name']} : public SuperRPCStub {{\n"
    stub_str += 'private:\n'
    stub_str += f"\tstd::unordered_map<RPCID, std::function<bool({IDLdata['settings']['stub_class_name']}&, NID, SerialBuffer*, SerialBufferIO*)>> __RPC__RPCDSPHMap;\n"
    stub_str += '''
private:
    virtual bool __RPC__DispatchRPC(NID nid, SerialBuffer* buf) {
        RPCID rpcid;
        SerialBufferIO bufio(buf);
        bufio >> rpcid;

        auto RPCDSPHIter = __RPC__RPCDSPHMap.find(rpcid);
        if (RPCDSPHIter == __RPC__RPCDSPHMap.end()) { return false; }
        auto RPCDSPHFunc = RPCDSPHIter->second;
        return RPCDSPHFunc(*this, nid, buf, &bufio);
    }
'''
    stub_str += '\tvirtual void __RPC__Init() {\n'
    for k, v in IDLdata['RPCs'].items():
        rpcid = 0
        try:
            rpcid = v['RPCID']
            RPCIDcnt = rpcid
        except KeyError:
            rpcid = RPCIDcnt
        RPCIDcnt += 1

        stub_str += f"\t\t__RPC__RPCDSPHMap.insert({{ {rpcid}, &{IDLdata['settings']['stub_class_name']}::__RPCDSPH__{k} }});\n"
    stub_str += '\t}\n'

    for k, v in IDLdata['RPCs'].items():
        isValuesExist = True
        values = None
        try:
            values = v['values']
        except KeyError:
            isValuesExist = False
        stub_str += f'	bool __RPCDSPH__{k}(NID __RPC__nid, SerialBuffer* __RPC__buf, SerialBufferIO* __RPC__pbufio) {{ \n'
        if isValuesExist:
            for vk, vv in values.items():
                stub_str += f'\t\t{vv.replace("&", "")} {vk};\n'
            for vk, vv in values.items():
                arr_sizes = CheckArrayExpression(vk)
                if arr_sizes is None:
                    stub_str += f'\t\t*__RPC__pbufio >> {vk};\n'
                else:
                    tab_cnt = 2
                    arr_sizes_num = len(arr_sizes)
                    for idx in range(0,arr_sizes_num):
                        for _ in range(0, tab_cnt):
                            stub_str += '\t'
                        rpc_idx_str = f'__RPC__idx_{idx}'
                        stub_str += f"for(int {rpc_idx_str}=0; {rpc_idx_str}<{arr_sizes[idx]}; {rpc_idx_str}++) {{\n"
                        tab_cnt += 1
                    for _ in range(0, tab_cnt):
                        stub_str += '\t'
                    stub_str += f"*__RPC__pbufio >> {ExtractArrayName(vk)}"
                    for idx in range(0,arr_sizes_num):
                        rpc_idx_str = f'__RPC__idx_{idx}'
                        stub_str += f"[{rpc_idx_str}]"
                    stub_str += ';\n'
                    for idx in range(0,arr_sizes_num):
                        tab_cnt -= 1
                        for _ in range(0, tab_cnt):
                            stub_str += '\t'
                        stub_str += '}\n'
                
        stub_str += f'\t\treturn {k}(__RPC__nid'
        if isValuesExist:
            for vk, vv in values.items():
                stub_str += f', {ExtractArrayName(vk)}'
        stub_str += ');\n'
        stub_str += '\t}\n'
    
    stub_str += 'protected:\n'
    for k, v in IDLdata['RPCs'].items():
        isValuesExist = True
        values = None
        try:
            values = v['values']
        except KeyError:
            isValuesExist = False
        stub_str += f'\tvirtual bool {k}(NID __RPC__nid'
        if isValuesExist:
            for vk, vv in values.items():
                arr_sizes = CheckArrayExpression(vk)
                if arr_sizes is None:
                    stub_str += f', {vv} {vk}'
                else:
                    stub_str += f', {vv} (&{ExtractArrayName(vk)})'
                    for arrsiz in arr_sizes:
                        stub_str += f'[{arrsiz}]'
        stub_str += ') { return false; }\n'
    stub_str += '};\n'

    if IDLdata['settings']['stub_make_param_arg_define']:
        stub_str += "\n"
        for k, v in IDLdata['RPCs'].items():
            isValuesExist = True
            values = None
            try:
                values = v['values']
            except KeyError:
                isValuesExist = False
            stub_str += f"#define dfRPC_STUB_PARAM_{IDLdata['settings']['stub_class_name']}_{k} NID __RPC__nid"
            if isValuesExist:
                for vk, vv in values.items():
                    arr_sizes = CheckArrayExpression(vk)
                    if arr_sizes is None:
                        stub_str += f', {vv} {vk}'
                    else:
                        stub_str += f', {vv} (&{ExtractArrayName(vk)})'
                        for arrsiz in arr_sizes:
                            stub_str += f'[{arrsiz}]'
                stub_str += '\n'
            else:
                stub_str += '\n'
        stub_str += '\n'

        for k, v in IDLdata['RPCs'].items():
            isValuesExist = True
            values = None
            try:
                values = v['values']
            except KeyError:
                isValuesExist = False
            stub_str += f"#define dfRPC_STUB_ARG_{IDLdata['settings']['stub_class_name']}_{k} __RPC__nid"
            if isValuesExist:
                for vk, vv in values.items():
                    stub_str += f', {ExtractArrayName(vk)}'
            stub_str += '\n'


    return stub_str

def doCompile(IDL_script_path):
    # load IDL Script via Json
    IDLdata = None
    with open(IDL_script_path, "r") as JsonFile:
        IDLdata = json.load(JsonFile)

    # build proxy and stub
    proxy_str = BuildProxy(IDLdata)
    stub_str = BuildStub(IDLdata)

    # print proxy and stub
    if IDLdata['settings']['print_code_at_console']:
        print('PROXY=================')
        print(proxy_str)
        print('STUB==================')
        print(stub_str)
        print('END===================')
    
    #write proxy and stub
    proxy_path = Path(IDLdata['settings']['proxy_dir_header_name'])
    stub_path = Path(IDLdata['settings']['stub_dir_header_name'])
    
    proxy_path.parent.mkdir(parents=True, exist_ok=True)
    stub_path.parent.mkdir(parents=True, exist_ok=True)
    
    print(f"Writing Proxy code at: {proxy_path.resolve()}")
    with open(proxy_path, "w") as proxyfile:
        proxyfile.write(proxy_str)
    print(f"Writing Stub code at: {stub_path.resolve()}")
    with open(stub_path, "w") as stubfile:
        stubfile.write(stub_str)
    
    return

def main(argc, argv):
    print('smolRPC IDL Compiler for CNetServer.')
    
    if argc == 1:
        print("At least 1 IDL script Json is required.")
        print("Please, pass IDL script Json file path via arguments.")
        return
    
    filecnt = 0
    for i in argv[1:]:
        print(f"===== Compiling IDL script: {i} =====")
        doCompile(i)
        print(f"===== Compile finished for: {i} =====")
        filecnt += 1
    
    print(f'Compiled {filecnt} IDL script(s).')
    print('smolRPC IDL Compiler finished.')
    return


if __name__ == '__main__':
    main(len(sys.argv), sys.argv)
