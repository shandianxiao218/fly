# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## AI Guidance

* Ignore GEMINI.md and GEMINI-*.md files
* To save main context space, for code searches, inspections, troubleshooting or analysis, use code-searcher subagent where appropriate - giving the subagent full context background for the task(s) you assign it.
* After receiving tool results, carefully reflect on their quality and determine optimal next steps before proceeding. Use your thinking to plan and iterate based on this new information, and then take the best next action.
* For maximum efficiency, whenever you need to perform multiple independent operations, invoke all relevant tools simultaneously rather than sequentially.
* Before you finish, please verify your solution
* Do what has been asked; nothing more, nothing less.
* NEVER create files unless they're absolutely necessary for achieving your goal.
* ALWAYS prefer editing an existing file to creating a new one.
* NEVER proactively create documentation files (*.md) or README files. Only create documentation files if explicitly requested by the User.
* When you update or modify core context files, also update markdown documentation and memory bank
* When asked to commit changes, exclude CLAUDE.md and CLAUDE-*.md referenced memory bank system files from any commits. Never delete these files.
* 工程的状态随时更新同步到“工程状态.md”文档中，每次更新要附上修改日期和时间；
* 增加必要的注释，注释用中文，格式满足doxygen的要求；
* 自动调用相应的subagent进行工作；
* 用code-reviewer对代码进行检查；
* 写代码用相应语言专用的subagent进行编写；
* 调试代码用debugger代理；
* 软件测试的过程用debug_log.md进行记录
* 需求追踪状态必须实时更新到specs/REQUIREMENTS-TRACKING.md，更新时机包括：
  - 完成功能实现时
  - 通过测试验证时  
  - 遇到问题或阻塞时
  - 每周项目状态更新时
  - 模块间依赖关系发生变化时
* 
## Memory Bank System

This project uses a structured memory bank system with specialized context files. Always check these files for relevant information before starting work:

### Core Context Files

* **CLAUDE-activeContext.md** - Current session state, goals, and progress (if exists)
* **CLAUDE-patterns.md** - Established code patterns and conventions (if exists)
* **CLAUDE-decisions.md** - Architecture decisions and rationale (if exists)
* **CLAUDE-troubleshooting.md** - Common issues and proven solutions (if exists)
* **CLAUDE-config-variables.md** - Configuration variables reference (if exists)
* **CLAUDE-temp.md** - Temporary scratch pad (created when needed)

**Important:** Always reference the active context file first to understand what's currently being worked on and maintain session continuity.

### Requirements Tracking

* **specs/REQUIREMENTS.md** - Complete requirements specification
* **specs/REQUIREMENTS-TRACKING.md** - Requirements implementation status and progress tracking
* **specs/REQUIREMENTS-UPDATE-TEMPLATE.md** - Quick update template and checklist

### Memory Bank System Backups

When asked to backup Memory Bank System files, you will copy the core context files above and @.claude settings directory to directory @/path/to/backup-directory. If files already exist in the backup directory, you will overwrite them.

## Project Overview
Project: 北斗导航卫星可见性分析系统 - 核心模块已完成，系统集成测试通过
Created: 2025-08-13
Generated using cc-sdd workflow
Current Status: satellite和aircraft模块完成，系统集成测试通过
Last Updated: 2025-09-06
-----
