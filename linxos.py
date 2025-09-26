#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
LinX OS SDK 统一编译工具
支持项目选择、配置和自动编译
"""

import os
import sys
import json
import argparse
import subprocess
import shutil
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# 颜色输出
class Colors:
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    CYAN = '\033[0;36m'
    MAGENTA = '\033[0;35m'
    NC = '\033[0m'  # No Color

def log_info(msg: str):
    print(f"{Colors.BLUE}[LinX OS]{Colors.NC} {msg}")

def log_warn(msg: str):
    print(f"{Colors.YELLOW}[LinX OS]{Colors.NC} {msg}")

def log_error(msg: str):
    print(f"{Colors.RED}[LinX OS]{Colors.NC} {msg}")

def log_success(msg: str):
    print(f"{Colors.GREEN}[LinX OS]{Colors.NC} {msg}")

class LinxOSBuilder:
    """LinX OS 统一编译工具"""
    
    def __init__(self):
        self.sdk_path = Path(__file__).parent.absolute()
        self.build_dir = self.sdk_path / "build"
        self.out_dir = self.sdk_path / "out"
        self.config_file = self.sdk_path / "sdkconfig"
        self.configs_dir = self.build_dir / "configs"
        
        # 确保目录存在
        self.build_dir.mkdir(exist_ok=True)
        self.out_dir.mkdir(exist_ok=True)
        self.configs_dir.mkdir(exist_ok=True)
        
        # 项目配置
        self.projects = self._scan_projects()
        self.available_configs = self._scan_configs()
        self.current_config = None  # 先设为None
        self.current_config = self._load_config()  # 再加载配置
    
    def _scan_configs(self) -> Dict:
        """扫描可用的配置文件"""
        configs = {}
        
        if self.configs_dir.exists():
            for config_file in self.configs_dir.glob("*.config"):
                config_name = config_file.stem
                config_data = self._parse_config_file(config_file)
                if config_data:
                    configs[config_name] = {
                        "file": str(config_file),
                        "data": config_data,
                        "description": config_data.get("CONFIG_DESCRIPTION", config_name)
                    }
        
        return configs
    
    def _parse_config_file(self, config_file: Path) -> Dict:
        """解析配置文件"""
        config_data = {}
        
        try:
            with open(config_file, 'r', encoding='utf-8') as f:
                for line in f:
                    line = line.strip()
                    if line and not line.startswith('#'):
                        if '=' in line:
                            key, value = line.split('=', 1)
                            # 去除引号
                            value = value.strip('"\'')
                            config_data[key] = value
        except Exception as e:
            log_warn(f"解析配置文件失败 {config_file}: {e}")
            return {}
        
        return config_data

    def _scan_projects(self) -> Dict:
        """扫描可用的项目"""
        projects = {
            "apps": {},
            "examples": {}
        }
        
        # 扫描 apps 目录
        apps_dir = self.sdk_path / "apps"
        if apps_dir.exists():
            for item in apps_dir.iterdir():
                if item.is_dir():
                    projects["apps"][item.name] = {
                        "path": str(item),
                        "type": "app",
                        "platform": self._detect_platform(item)
                    }
        
        # 扫描 examples 目录
        examples_dir = self.sdk_path / "examples"
        if examples_dir.exists():
            for item in examples_dir.iterdir():
                if item.is_dir():
                    projects["examples"][item.name] = {
                        "path": str(item),
                        "type": "example",
                        "platform": item.name  # examples 目录名就是平台名
                    }
        
        return projects
    
    def _detect_platform(self, project_path: Path) -> str:
        """检测项目平台"""
        # 检查是否有特定的配置文件
        if (project_path / "sdkconfig").exists():
            return "esp32"
        elif (project_path / "CMakeLists.txt").exists():
            return "native"
        else:
            return "unknown"
    
    def _load_config(self) -> Dict:
        """加载当前配置"""
        config = {
            "target": "native",
            "board": "mac",
            "build_type": "Release",
            "toolchain_file": "",
            "toolchain_prefix": "",
            "description": "默认配置",
            "sdk_built": False,
            "board_built": False
        }
        
        if self.config_file.exists():
            try:
                config_data = self._parse_config_file(self.config_file)
                if config_data:
                    config.update({
                        "target": config_data.get("CONFIG_TARGET_PLATFORM", "native"),
                        "board": config_data.get("CONFIG_BOARD_PLATFORM", "mac"),
                        "build_type": config_data.get("CONFIG_BUILD_TYPE", "Release"),
                        "toolchain_file": config_data.get("CONFIG_TOOLCHAIN_FILE", ""),
                        "toolchain_prefix": config_data.get("CONFIG_TOOLCHAIN_PREFIX", ""),
                        "description": config_data.get("CONFIG_DESCRIPTION", "当前配置")
                    })
            except Exception as e:
                log_warn(f"读取配置文件失败: {e}")
        
        # 检查SDK和Board是否已编译
        config["sdk_built"] = self._check_sdk_built()
        config["board_built"] = self._check_board_built()
        
        return config
    
    def _check_sdk_built(self) -> bool:
        """检查SDK是否已编译"""
        sdk_lib = self.out_dir / "linx" / "lib" / "liblinx_sdk_static.a"
        return sdk_lib.exists()
    
    def _check_board_built(self) -> bool:
        """检查Board是否已编译"""
        # 如果current_config还未初始化，使用默认值
        if self.current_config is None:
            board = "mac"
        else:
            board = self.current_config.get("board", "mac")
        board_lib = self.out_dir / "linx" / "lib" / f"liblinx_board_{board}.a"
        return board_lib.exists()
    
    def list_projects(self):
        """列出所有可用项目"""
        print(f"\n{Colors.CYAN}可用项目列表:{Colors.NC}")
        print("=" * 60)
        
        # 显示 Apps
        if self.projects["apps"]:
            print(f"\n{Colors.GREEN}应用程序 (Apps):{Colors.NC}")
            for name, info in self.projects["apps"].items():
                platform = info["platform"]
                print(f"  {Colors.YELLOW}{name:20}{Colors.NC} - 平台: {platform}")
        
        # 显示 Examples
        if self.projects["examples"]:
            print(f"\n{Colors.GREEN}示例程序 (Examples):{Colors.NC}")
            for name, info in self.projects["examples"].items():
                platform = info["platform"]
                print(f"  {Colors.YELLOW}{name:20}{Colors.NC} - 平台: {platform}")
        
        print()
    
    def config_choice(self):
        """配置选择界面"""
        print(f"\n{Colors.CYAN}LinX OS SDK 配置选择:{Colors.NC}")
        print("=" * 60)
        
        # 显示当前配置
        print(f"当前配置:")
        print(f"  配置名称: {self.current_config.get('description', '未知')}")
        print(f"  目标平台: {self.current_config['target']}")
        print(f"  板框平台: {self.current_config['board']}")
        print(f"  构建类型: {self.current_config['build_type']}")
        print(f"  工具链文件: {self.current_config.get('toolchain_file', '无')}")
        print(f"  SDK状态: {'已编译' if self.current_config['sdk_built'] else '未编译'}")
        print(f"  Board状态: {'已编译' if self.current_config['board_built'] else '未编译'}")
        
        # 显示可用配置
        if not self.available_configs:
            log_warn("未找到可用的配置文件")
            return False
        
        print(f"\n{Colors.GREEN}可用配置选项:{Colors.NC}")
        print("-" * 60)
        
        config_list = list(self.available_configs.items())
        for i, (config_name, config_info) in enumerate(config_list, 1):
            description = config_info["description"]
            target = config_info["data"].get("CONFIG_TARGET_PLATFORM", "unknown")
            board = config_info["data"].get("CONFIG_BOARD_PLATFORM", "unknown")
            print(f"  {i:2d}. {config_name:15} - {description}")
            print(f"      平台: {target:10} 板框: {board}")
        
        print("-" * 60)
        print('输入 "q" 退出配置选择')
        
        try:
            choice = input(f"\n请选择配置 (1-{len(config_list)}, 回车保持当前配置): ").strip()
            
            if choice.lower() == 'q':
                log_info("退出配置选择")
                return False
            
            if choice:
                try:
                    config_idx = int(choice) - 1
                    if 0 <= config_idx < len(config_list):
                        config_name, config_info = config_list[config_idx]
                        return self._apply_config(config_name, config_info)
                    else:
                        log_error("无效选择")
                        return False
                except ValueError:
                    log_error("请输入有效的数字")
                    return False
            else:
                log_info("保持当前配置")
                return True
                
        except (KeyboardInterrupt, EOFError):
            log_error("\n配置被取消")
            return False
    
    def _apply_config(self, config_name: str, config_info: Dict) -> bool:
        """应用选定的配置"""
        try:
            config_data = config_info["data"]
            
            # 复制配置文件内容到sdkconfig
            shutil.copy2(config_info["file"], self.config_file)
            
            # 更新当前配置
            self.current_config.update({
                "target": config_data.get("CONFIG_TARGET_PLATFORM", "native"),
                "board": config_data.get("CONFIG_BOARD_PLATFORM", "mac"),
                "build_type": config_data.get("CONFIG_BUILD_TYPE", "Release"),
                "toolchain_file": config_data.get("CONFIG_TOOLCHAIN_FILE", ""),
                "toolchain_prefix": config_data.get("CONFIG_TOOLCHAIN_PREFIX", ""),
                "description": config_data.get("CONFIG_DESCRIPTION", config_name)
            })
            
            # 重新检查编译状态
            self.current_config["sdk_built"] = self._check_sdk_built()
            self.current_config["board_built"] = self._check_board_built()
            
            log_success(f"已选择配置: {config_name}")
            log_info(f"配置描述: {config_info['description']}")
            
            # 如果切换了配置，提示可能需要重新编译
            if not self.current_config["sdk_built"] or not self.current_config["board_built"]:
                log_warn("配置已更改，建议重新编译SDK和Board")
                log_info("使用 './linxos.py build-sdk -f' 和 './linxos.py build-board -f' 重新编译")
            
            return True
            
        except Exception as e:
            log_error(f"应用配置失败: {e}")
            return False

    def build_sdk(self, force: bool = False) -> bool:
        """编译SDK"""
        if self.current_config["sdk_built"] and not force:
            log_info("SDK已编译，跳过编译步骤")
            return True
        
        log_info("开始编译SDK...")
        
        try:
            sdk_dir = self.sdk_path / "sdk"
            sdk_build_dir = sdk_dir / "build"
            
            # 清理构建目录（如果强制重新编译）
            if force and sdk_build_dir.exists():
                shutil.rmtree(sdk_build_dir)
            
            sdk_build_dir.mkdir(exist_ok=True)
            
            # 配置CMake
            cmake_args = [
                "cmake",
                f"-DCMAKE_BUILD_TYPE={self.current_config['build_type']}"
            ]
            
            # 添加工具链文件（如果有）
            toolchain_file = self.current_config.get('toolchain_file')
            if toolchain_file:
                toolchain_path = self.build_dir / "toolchains" / toolchain_file
                if toolchain_path.exists():
                    cmake_args.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_path}")
                    log_info(f"使用工具链文件: {toolchain_file}")
                else:
                    log_warn(f"工具链文件不存在: {toolchain_path}")
            
            cmake_args.append(str(sdk_dir))
            
            log_info(f"配置SDK: {' '.join(cmake_args)}")
            result = subprocess.run(cmake_args, cwd=sdk_build_dir)
            if result.returncode != 0:
                log_error("SDK CMake配置失败")
                return False
            
            # 编译SDK
            make_args = ["make", "-j", str(os.cpu_count() or 4)]
            log_info(f"编译SDK: {' '.join(make_args)}")
            result = subprocess.run(make_args, cwd=sdk_build_dir)
            
            if result.returncode != 0:
                log_error("SDK编译失败")
                return False
            
            # 安装SDK
            install_args = ["make", "install"]
            log_info(f"安装SDK: {' '.join(install_args)}")
            result = subprocess.run(install_args, cwd=sdk_build_dir)
            
            if result.returncode == 0:
                self.current_config["sdk_built"] = True
                log_success("SDK编译成功")
                return True
            else:
                log_error("SDK安装失败")
                return False
                    
        except Exception as e:
            log_error(f"SDK编译过程出错: {e}")
            return False
    
    def build_board(self, force: bool = False) -> bool:
        """编译Board适配"""
        if not self.current_config["sdk_built"]:
            log_error("请先编译SDK")
            return False
        
        if self.current_config["board_built"] and not force:
            log_info("Board已编译，跳过编译步骤")
            return True
        
        log_info("开始编译Board适配...")
        
        try:
            board = self.current_config.get("board", "mac")
            board_dir = self.sdk_path / "board" / board
            
            if not board_dir.exists():
                log_error(f"Board目录不存在: {board_dir}")
                return False
            
            board_build_dir = board_dir / "build"
            
            # 清理构建目录（如果强制重新编译）
            if force and board_build_dir.exists():
                shutil.rmtree(board_build_dir)
            
            board_build_dir.mkdir(exist_ok=True)
            
            # 配置CMake
            cmake_args = [
                "cmake",
                f"-DCMAKE_BUILD_TYPE={self.current_config['build_type']}"
            ]
            
            # 添加工具链文件（如果有）
            toolchain_file = self.current_config.get('toolchain_file')
            if toolchain_file:
                toolchain_path = self.build_dir / "toolchains" / toolchain_file
                if toolchain_path.exists():
                    cmake_args.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_path}")
                    log_info(f"使用工具链文件: {toolchain_file}")
            
            cmake_args.append(str(board_dir))
            
            log_info(f"配置Board: {' '.join(cmake_args)}")
            result = subprocess.run(cmake_args, cwd=board_build_dir)
            if result.returncode != 0:
                log_error("Board CMake配置失败")
                return False
            
            # 编译Board
            make_args = ["make", "-j", str(os.cpu_count() or 4)]
            log_info(f"编译Board: {' '.join(make_args)}")
            result = subprocess.run(make_args, cwd=board_build_dir)
            
            if result.returncode != 0:
                log_error("Board编译失败")
                return False
            
            # 安装Board
            install_args = ["make", "install"]
            log_info(f"安装Board: {' '.join(install_args)}")
            result = subprocess.run(install_args, cwd=board_build_dir)
            
            if result.returncode == 0:
                self.current_config["board_built"] = True
                log_success("Board编译成功")
                return True
            else:
                log_error("Board安装失败")
                return False
                
        except Exception as e:
            log_error(f"Board编译过程出错: {e}")
            return False

    def build_all(self, force: bool = False) -> bool:
        """编译所有组件 (SDK + Board + Examples)"""
        log_info("开始编译所有组件...")
        
        # 编译SDK
        if not self.build_sdk(force):
            log_error("SDK编译失败，停止编译")
            return False
        
        # 编译Board
        if not self.build_board(force):
            log_error("Board编译失败，停止编译")
            return False
        
        # 编译Examples
        if not self.build_examples(force):
            log_error("Examples编译失败，停止编译")
            return False
        
        log_success("所有组件编译成功")
        return True

    def build_examples(self, force: bool = False) -> bool:
        """编译所有示例程序"""
        log_info("开始编译示例程序...")
        
        if not self.projects["examples"]:
            log_warn("没有找到示例程序")
            return True
        
        success_count = 0
        total_count = len(self.projects["examples"])
        
        for example_name in self.projects["examples"]:
            log_info(f"编译示例: {example_name}")
            if self.build_project("examples", example_name, force):
                success_count += 1
            else:
                log_warn(f"示例编译失败: {example_name}")
        
        if success_count == total_count:
            log_success(f"所有示例编译成功 ({success_count}/{total_count})")
            return True
        else:
            log_warn(f"部分示例编译失败 ({success_count}/{total_count})")
            return False

    def build_project(self, project_type: str, project_name: str, force: bool = False) -> bool:
        """编译指定项目"""
        if project_type not in self.projects:
            log_error(f"无效的项目类型: {project_type}")
            return False
        
        if project_name not in self.projects[project_type]:
            log_error(f"项目不存在: {project_type}/{project_name}")
            return False
        
        project_info = self.projects[project_type][project_name]
        project_path = Path(project_info["path"])
        
        log_info(f"开始编译项目: {project_type}/{project_name}")
        
        # 确保SDK和Board已编译
        if not self.build_sdk():
            return False
        
        if not self.build_board():
            return False
        
        # 编译项目
        try:
            build_dir = project_path / "build"
            if force and build_dir.exists():
                shutil.rmtree(build_dir)
            
            build_dir.mkdir(exist_ok=True)
            
            # 配置CMake
            cmake_args = [
                "cmake",
                f"-DCMAKE_BUILD_TYPE={self.current_config['build_type']}",
                str(project_path)
            ]
            
            log_info(f"配置项目: {' '.join(cmake_args)}")
            result = subprocess.run(cmake_args, cwd=build_dir)
            if result.returncode != 0:
                log_error("CMake配置失败")
                return False
            
            # 编译项目
            make_args = ["make", "-j", str(os.cpu_count() or 4)]
            log_info(f"编译项目: {' '.join(make_args)}")
            result = subprocess.run(make_args, cwd=build_dir)
            
            if result.returncode == 0:
                log_success(f"项目编译成功: {project_type}/{project_name}")
                
                # 显示可执行文件位置
                bin_dir = build_dir / "bin"
                if bin_dir.exists():
                    for exe in bin_dir.iterdir():
                        if exe.is_file() and os.access(exe, os.X_OK):
                            log_info(f"可执行文件: {exe}")
                
                return True
            else:
                log_error("项目编译失败")
                return False
                
        except Exception as e:
            log_error(f"项目编译过程出错: {e}")
            return False
    
    def run_project(self, project_type: str, project_name: str, args: List[str] = None):
        """运行指定项目"""
        if project_type not in self.projects:
            log_error(f"无效的项目类型: {project_type}")
            return False
        
        if project_name not in self.projects[project_type]:
            log_error(f"项目不存在: {project_type}/{project_name}")
            return False
        
        project_info = self.projects[project_type][project_name]
        project_path = Path(project_info["path"])
        build_dir = project_path / "build"
        
        # 查找可执行文件
        exe_file = None
        bin_dir = build_dir / "bin"
        if bin_dir.exists():
            for exe in bin_dir.iterdir():
                if exe.is_file() and os.access(exe, os.X_OK):
                    exe_file = exe
                    break
        
        if not exe_file:
            log_error(f"未找到可执行文件，请先编译项目")
            return False
        
        # 运行项目
        try:
            cmd = [str(exe_file)]
            if args:
                cmd.extend(args)
            
            log_info(f"运行项目: {' '.join(cmd)}")
            result = subprocess.run(cmd)
            return result.returncode == 0
            
        except Exception as e:
            log_error(f"运行项目失败: {e}")
            return False
    
    def clean(self, project_type: str = None, project_name: str = None):
        """清理编译文件"""
        if project_type and project_name:
            # 清理指定项目
            if project_type in self.projects and project_name in self.projects[project_type]:
                project_path = Path(self.projects[project_type][project_name]["path"])
                build_dir = project_path / "build"
                if build_dir.exists():
                    shutil.rmtree(build_dir)
                    log_success(f"已清理项目: {project_type}/{project_name}")
                else:
                    log_info(f"项目无需清理: {project_type}/{project_name}")
            else:
                log_error(f"项目不存在: {project_type}/{project_name}")
        else:
            # 清理所有
            log_info("清理所有编译文件...")
            
            # 清理SDK构建文件
            if self.build_dir.exists():
                shutil.rmtree(self.build_dir)
                log_info("已清理SDK构建文件")
            
            # 清理输出文件
            if self.out_dir.exists():
                shutil.rmtree(self.out_dir)
                log_info("已清理输出文件")
            
            # 清理项目构建文件
            for project_type in self.projects:
                for project_name, project_info in self.projects[project_type].items():
                    project_path = Path(project_info["path"])
                    build_dir = project_path / "build"
                    if build_dir.exists():
                        shutil.rmtree(build_dir)
                        log_info(f"已清理项目: {project_type}/{project_name}")
            
            # 重置状态
            self.current_config["sdk_built"] = False
            self.current_config["board_built"] = False
            
            log_success("清理完成")

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description="LinX OS SDK 统一编译工具")
    subparsers = parser.add_subparsers(dest="command", help="可用命令")
    
    # 列出项目
    subparsers.add_parser("list", help="列出所有可用项目")
    
    # 配置选择
    config_parser = subparsers.add_parser("config", help="配置选择")
    config_subparsers = config_parser.add_subparsers(dest="config_command")
    config_subparsers.add_parser("choice", help="交互式配置选择")
    
    # 编译命令
    build_parser = subparsers.add_parser("build", help="编译项目")
    build_parser.add_argument("target", help="编译目标: sdk, board, all, examples, apps")
    build_parser.add_argument("project_name", nargs="?", help="项目名称 (当target为apps/examples时必需)")
    build_parser.add_argument("-f", "--force", action="store_true", help="强制重新编译")
    
    # 保持向后兼容的单独命令
    sdk_parser = subparsers.add_parser("build-sdk", help="编译SDK (已弃用，请使用 build sdk)")
    sdk_parser.add_argument("-f", "--force", action="store_true", help="强制重新编译")
    
    board_parser = subparsers.add_parser("build-board", help="编译Board适配 (已弃用，请使用 build board)")
    board_parser.add_argument("-f", "--force", action="store_true", help="强制重新编译")
    
    # 运行项目
    run_parser = subparsers.add_parser("run", help="运行项目")
    run_parser.add_argument("project_type", choices=["apps", "examples"], help="项目类型")
    run_parser.add_argument("project_name", help="项目名称")
    run_parser.add_argument("args", nargs="*", help="程序参数")
    
    # 清理
    clean_parser = subparsers.add_parser("clean", help="清理编译文件")
    clean_parser.add_argument("project_type", nargs="?", choices=["apps", "examples"], help="项目类型")
    clean_parser.add_argument("project_name", nargs="?", help="项目名称")
    
    args = parser.parse_args()
    
    builder = LinxOSBuilder()
    
    if args.command == "list":
        builder.list_projects()
    elif args.command == "config":
        if hasattr(args, 'config_command') and args.config_command == "choice":
            builder.config_choice()
        else:
            builder.config_choice()
    elif args.command == "build-sdk":
        log_warn("build-sdk 命令已弃用，请使用 'build sdk'")
        builder.build_sdk(args.force)
    elif args.command == "build-board":
        log_warn("build-board 命令已弃用，请使用 'build board'")
        builder.build_board(args.force)
    elif args.command == "build":
        if args.target == "sdk":
            builder.build_sdk(args.force)
        elif args.target == "board":
            builder.build_board(args.force)
        elif args.target == "all":
            builder.build_all(args.force)
        elif args.target == "examples":
            builder.build_examples(args.force)
        elif args.target == "apps":
            if not args.project_name:
                log_error("编译apps时必须指定项目名称")
                return
            builder.build_project("apps", args.project_name, args.force)
        elif args.target in ["apps", "examples"]:
            if not args.project_name:
                log_error(f"编译{args.target}时必须指定项目名称")
                return
            builder.build_project(args.target, args.project_name, args.force)
        else:
            # 尝试作为项目类型和名称处理 (向后兼容)
            if args.target in ["apps", "examples"] and args.project_name:
                builder.build_project(args.target, args.project_name, args.force)
            else:
                log_error(f"无效的编译目标: {args.target}")
                log_info("可用目标: sdk, board, all, examples, apps <project_name>")
    elif args.command == "run":
        builder.run_project(args.project_type, args.project_name, args.args)
    elif args.command == "clean":
        builder.clean(args.project_type, args.project_name)
    else:
        # 默认显示项目列表
        builder.list_projects()
        print(f"\n{Colors.CYAN}使用示例:{Colors.NC}")
        print("  linxos.py config choice          # 配置选择")
        print("  linxos.py build sdk              # 编译SDK")
        print("  linxos.py build board            # 编译Board适配")
        print("  linxos.py build all              # 编译所有组件")
        print("  linxos.py build examples         # 编译所有示例")
        print("  linxos.py build examples mac     # 编译mac示例")
        print("  linxos.py run examples mac       # 运行mac示例")
        print("  linxos.py clean                  # 清理所有")

if __name__ == "__main__":
    main()