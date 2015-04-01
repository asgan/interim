#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "sledge/minilisp.h"
#include "sledge/alloc.h"
#include "sledge/blit.h"
#include "rpi2/raspi.h"
#include "rpi2/r3d.h"

#include <lightning.h>

void main();

extern uint32_t _bss_end;
uint8_t* heap_end;

void _cstartup(unsigned int r0, unsigned int r1, unsigned int r2)
{
  heap_end = (uint8_t*)0x1000000; // start allocating at 16MB
  memset(heap_end,0,1024*1024*16); // clear 16 MB of memory
  main();
  while(1) {};
}

// GPIO Register set
volatile unsigned int* gpio;

uint32_t* FB;
uint32_t* FB_MEM;

char buf[128];

void enable_mmu(void);
extern void* _get_stack_pointer();
void uart_repl();

void main()
{
  enable_mmu();
  arm_invalidate_data_caches();
  
  uart_puts("-- BOMBERJACKET/PI kernel_main entered.\r\n");
  setbuf(stdout, NULL);

  init_rpi_qpu();
  uart_puts("-- QPU enabled.\r\n");

  FB = init_rpi_gfx();
  FB_MEM = FB; //malloc(1920*1080*4);

  init_blitter(FB);
  
  sprintf(buf, "-- framebuffer at %p.\r\n",FB);
  uart_puts(buf);
  
  sprintf(buf, "-- heap starts at %p.\r\n", heap_end);
  uart_puts(buf);
  
  sprintf(buf, "-- stack pointer at %p.\r\n", _get_stack_pointer());
  uart_puts(buf);

  memset(FB, 0x88, 1920*1080*4);

  uart_repl();
}

void printhex(uint32_t num) {
  char buf[9];
  buf[8] = 0;
  for (int i=7; i>=0; i--) {
    int d = num&0xf;
    if (d<10) buf[i]='0'+d;
    else buf[i]='a'+d-10;
    num=num>>4;
  }
  uart_puts(buf);
}

void printhex_signed(int32_t num) {
  char buf[9];
  buf[8] = 0;
  if (num<0) {
    uart_putc('-');
    num=-num;
  }
  for (int i=7; i>=0; i--) {
    int d = num&0xf;
    if (d<10) buf[i]='0'+d;
    else buf[i]='a'+d-10;
    num=num/16;
  }
  uart_puts(buf);
}

#include "libc_glue.c"

int machine_video_set_pixel(uint32_t x, uint32_t y, uint32_t color) {
  if (x>=1920 || y>=1080) return 0;
  FB_MEM[y*1920+x] = color;
  
  return 0;
}

int machine_video_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
  uint32_t y1=y;
  uint32_t y2=y+h;
  uint32_t x2=x+w;
  uint32_t off = y1*1920;

  // FIXME: clip!
  
  for (; y1<y2; y1++) {
    for (uint32_t x1=x; x1<x2; x1++) {
      FB_MEM[off+x1] = color;
    }
    off+=1920;
  }

  return 0;
}

Cell* lookup_global_symbol(char* name);

int machine_video_flip() {
  nv_vertex_t* triangles = r3d_init_frame();

  Cell* c_x1 = lookup_global_symbol("tx1");
  Cell* c_x2 = lookup_global_symbol("tx2");
  Cell* c_y1 = lookup_global_symbol("ty1");
  Cell* c_y2 = lookup_global_symbol("ty2");
  
  int x1 = c_x1->value*16;
  int y1 = c_y1->value*16;
  int x2 = c_x2->value*16;
  int y2 = c_y2->value*16;

  triangles[0].x = x1;
  triangles[0].y = y1;
  triangles[0].z = 1.0f;
  triangles[0].w = 1.0f;
  triangles[0].r = 1.0f;
  triangles[0].g = 0.0f;
  triangles[0].b = 1.0f;
  
  triangles[1].x = x2;
  triangles[1].y = y1;
  triangles[1].z = 1.0f;
  triangles[1].w = 1.0f;
  triangles[1].r = 1.0f;
  triangles[1].g = 1.0f;
  triangles[1].b = 1.0f;
  
  triangles[2].x = x1;
  triangles[2].y = y2;
  triangles[2].z = 1.0f;
  triangles[2].w = 1.0f;
  triangles[2].r = 1.0f;
  triangles[2].g = 1.0f;
  triangles[2].b = 1.0f;
  
  triangles[3].x = x2;
  triangles[3].y = y1;
  triangles[3].z = 1.0f;
  triangles[3].w = 1.0f;
  triangles[3].r = 1.0f;
  triangles[3].g = 0.0f;
  triangles[3].b = 1.0f;
  
  triangles[4].x = x2;
  triangles[4].y = y2;
  triangles[4].z = 1.0f;
  triangles[4].w = 1.0f;
  triangles[4].r = 1.0f;
  triangles[4].g = 1.0f;
  triangles[4].b = 1.0f;
  
  triangles[5].x = x1;
  triangles[5].y = y2;
  triangles[5].z = 1.0f;
  triangles[5].w = 1.0f;
  triangles[5].r = 1.0f;
  triangles[5].g = 1.0f;
  triangles[5].b = 1.0f;
  
  r3d_triangles(2, triangles);
  r3d_render_frame(0xffffffff);
  
  //memset(FB_MEM, 0xffffff, 1920*1080*4);
  //r3d_debug_gpu();

  printf("-- r3d frame submitted.\r\n");
  
  return 0;
}

int machine_get_key(int modifiers) {
  if (modifiers) return 0;
  return uart_getc();
}

Cell* machine_poll_udp() {
  return NULL;
}

Cell* machine_send_udp(Cell* data_cell) {
  return NULL;
}

Cell* machine_connect_tcp(Cell* host_cell, Cell* port_cell, Cell* connected_fn_cell, Cell* data_fn_cell) {
  return NULL;
}

Cell* machine_bind_tcp(Cell* port_cell, Cell* fn_cell) {
  return NULL;
}

Cell* machine_send_tcp(Cell* data_cell) {
  return NULL;
}

Cell* machine_save_file(Cell* cell, char* path) {
  return alloc_int(0);
}

static char sysfs_tmp[1024];

Cell* machine_load_file(char* path) {
  // sysfs
  if (!strcmp(path,"/sys/mem")) {
    MemStats* mst = alloc_stats();
    sprintf(sysfs_tmp, "(%d %d %d %d)", mst->byte_heap_used, mst->byte_heap_max, mst->cells_used, mst->cells_max);
    return read_string(sysfs_tmp);
  }

  Cell* result_cell = alloc_num_bytes(2*1024*1024);
  return result_cell;
}

typedef jit_word_t (*funcptr)();
static jit_state_t *_jit;
static jit_state_t *_jit_saved;
static void *stack_ptr, *stack_base;

#include "sledge/compiler.c"

void insert_rootfs_symbols() {
  // until we have a file system, inject binaries that are compiled in the kernel
  // into the environment
  
  extern uint8_t _binary_bjos_rootfs_unifont_start;
  extern uint32_t _binary_bjos_rootfs_unifont_size;
  Cell* unif = alloc_bytes(16);
  unif->addr = &_binary_bjos_rootfs_unifont_start;
  unif->size = _binary_bjos_rootfs_unifont_size;

  printf("~~ unifont is at %p\r\n",unif->addr);

  insert_symbol(alloc_sym("unifont"), unif, &global_env);

  extern uint8_t _binary_bjos_rootfs_editor_l_start;
  extern uint32_t _binary_bjos_rootfs_editor_l_size;
  Cell* editor = alloc_string("editor");
  editor->addr = &_binary_bjos_rootfs_editor_l_start;
  editor->size = 0x1866; //_binary_bjos_rootfs_editor_l_size;

  printf("~~ editor-source is at %p\r\n",editor->addr);
  
  insert_symbol(alloc_sym("editor-source"), editor, &global_env);
  
  insert_symbol(alloc_sym("tx1"), alloc_int(800), &global_env);
  insert_symbol(alloc_sym("tx2"), alloc_int(900), &global_env);
  insert_symbol(alloc_sym("ty1"), alloc_int(800), &global_env);
  insert_symbol(alloc_sym("ty2"), alloc_int(900), &global_env);
}

void uart_repl() {
  char* out_buf = malloc(1024*10);
  char* in_line = malloc(1024*2);
  char* in_buf = malloc(1024*10);
  uart_puts("\r\n\r\nwelcome to sledge arm/32 (c)2015 mntmn.\r\n");
  
  init_compiler();
  insert_rootfs_symbols();

  uart_puts("\r\n\r\ncompiler initialized.\r\n");
  
  memset(out_buf,0,1024*10);
  memset(in_line,0,1024*2);
  memset(in_buf,0,1024*10);

  // jit stack
  stack_ptr = stack_base = malloc(4096 * sizeof(jit_word_t));

  long count = 0;  
  int fullscreen = 0;
  
  int in_offset = 0;
  int parens = 0;

  int linec = 0;

  Cell* expr;
  char c = 13;

  strcpy(in_line,"(eval editor-source)\n");
  
  init_jit(NULL);
  uart_puts("\r\n\r\n~~ JIT initialized.\r\n");

  r3d_init(FB);
  uart_puts("-- R3D initialized.\r\n");
  
  while (1) {
    expr = NULL;
    
    uart_puts("sledge> ");

    int i = 0;

    while (c!=13) {
      c = uart_getc();
      uart_putc(c);
      in_line[i++] = c;
      in_line[i] = 0;
    }
    c = 0;
    
    int len = strlen(in_line);

    // recognize parens
    
    for (i=0; i<len; i++) {
      if (in_line[i] == '(') {
        parens++;
      } else if (in_line[i] == ')') {
        parens--;
      }
    }

    //printf("parens: %d in_offset: %d\n",parens,in_offset);

    if (len>1) {
      strncpy(in_buf+in_offset, in_line, len-1);
      in_buf[in_offset+len-1] = 0;
      
      //printf("line: '%s' (%d)\n",in_buf,strlen(in_buf));
      
      linec++;
      //if (linec>10) while (1) {};
    }
    printf("\r\n[%s]\r\n",in_buf);
    
    if (parens>0) {
      printf("\r\n...\r\n");
      in_offset+=len-1;
    } else {
      if (len>1) {
        expr = read_string(in_buf);
        in_offset=0;
      }
    }
    //printf("parens: %d offset: %d\n",parens,in_offset);
    
    jit_node_t  *in;
    funcptr     compiled;

    if (expr) {
      _jit = jit_new_state();
      
      jit_prolog();
      
      int success = compile_arg(JIT_R0, expr, TAG_ANY);

      jit_retr(JIT_R0);

      if (success) {
        compiled = jit_emit();

        //jit_disassemble();

        //start_clock();

        printf("-- compiled: %p\r\n",compiled);
        memdump((uint32_t)compiled,200,1);

        Cell* res = (Cell*)compiled();
        printf("-- res at: %p\r\n",res);

        // TODO: move to write op
        if (!res) {
          uart_puts("null\n");
        } else {
          lisp_write(res, out_buf, 1024*10);
          uart_puts(out_buf);
        }
      }
      
      uart_puts("\r\n");
      
      jit_clear_state();
      jit_destroy_state();
    }
  }
}
