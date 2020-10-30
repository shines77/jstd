See: https://www.cnblogs.com/sutrahsing/p/11752092.html

See: https://www.cnblogs.com/cu-later/p/13802011.html

不更新指定的文件，但是保留远程仓库中的文件不变，即本地可以随意修改，但不更新到远端，也不影响远端的该文件。
（执行命令之前需要保持该文件为已同步状态（已存在于远端），否则执行失败）

	git update-index --assume-unchanged ./src/main/jstd/basic/vld_def.h

如果需要取消不更新，则执行：

	git update-index --no-assume-unchanged ./src/main/jstd/basic/vld_def.h

删除本地缓存文件：

	git rm --cached ./src/main/jstd/basic/vld_def.h
