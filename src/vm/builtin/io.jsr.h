// WARNING: this is a file generated automatically by the build process from
// "/home/fabrizio/Workspace/c/blang/src/vm/builtin/io.jsr". Do not modify.
const char *io_jsr =
"class IOException : Exception end\n"
"class FileNotFoundException : IOException end\n"
"var SEEK_SET, SEEK_CUR, SEEK_END = 0, 1, 2\n"
"class File\n"
"    native new(path, mode)\n"
"    native tell()\n"
"    native seek(off, whence=0)\n"
"    native rewind()\n"
"    native read(bytes)\n"
"    native readAll()\n"
"    native readLine()\n"
"    native write(data)\n"
"    native close()\n"
"    native flush()\n"
"    fun size()\n"
"        var oldpos = this.tell()\n"
"        this.seek(0, SEEK_END)\n"
"        var size = this.tell()\n"
"        this.seek(oldpos)\n"
"        return size\n"
"    end\n"
"    fun __iter__(_)\n"
"        return this.readLine()\n"
"    end\n"
"    fun __next__(line)\n"
"        return line\n"
"    end\n"
"    fun __string__()\n"
"        return \"<\" + (\"closed \" if this._closed else \"open \") + super.__string__() + \">\"\n"
"    end\n"
"end\n"
"class __PFile : File\n"
"    fun new(handle)\n"
"        this._handle = handle\n"
"        this._closed = false\n"
"    end\n"
"    native close()\n"
"end\n"
"native popen(name, mode=\"r\")\n"
"native remove(path)\n"
"native rename(oldpath, newpath)\n"
;