OUTPUT_ARCH( "riscv" )
ENTRY( _entry )  // 指定程序的入口

// 描述文件的内存布局
SECTIONS
{
  /*
   * ensure that entry.S / _entry is at 0x80000000,
   * where qemu's -kernel jumps.
   */
  . = 0x80000000;  // 设置当前位置

  .text : {   // 代码段
    // *: 通配符，匹配所有文件。即将所有文件的 .text 段和 .text.* 段的内容合并到这个段中
    *(.text .text.*)  
    . = ALIGN(0x1000);  // 向上对齐到 0x1000 的边界，即对齐内存页面（4096 字节大小）
    _trampoline = .;     // 将 . 的值赋值给 _trampoline
    *(trampsec)  // 将所有文件的 trampsec 段合并到这个段中
    . = ALIGN(0x1000); // 再次对齐边界
    // 检查 trampoline 的大小
    ASSERT(. - _trampoline == 0x1000, "error: trampoline larger than one page");
    PROVIDE(etext = .);   // etext means end of text。
  }

  // 只读的数据段
  .rodata : {
    . = ALIGN(16);
    // .srodata .data .sbss 段使用短地址访问，将常用的数据放入这些段中可减少代码文件大小
    *(.srodata .srodata.*) /* do not need to distinguish this from .rodata */
    . = ALIGN(16);
    *(.rodata .rodata.*)
  }

  .data : {
    . = ALIGN(16);
    *(.sdata .sdata.*) /* do not need to distinguish this from .data */
    . = ALIGN(16);
    *(.data .data.*)
  }

  // 程序的未初始化数据段，用于存储未初始化的全局变量和静态变量
  .bss : {
    . = ALIGN(16);
    *(.sbss .sbss.*) /* do not need to distinguish this from .bss */
    . = ALIGN(16);
    *(.bss .bss.*)
  }

  PROVIDE(end = .);
}
