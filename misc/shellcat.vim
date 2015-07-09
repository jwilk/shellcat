if exists("b:current_syntax")
    finish
endif

syn include @shellcatShellScript syntax/sh.vim

syn match shellcatComment '\%^#!.*'
syn region shellcatCode matchgroup=Operator start='<\$[^-]\@=' keepend end='-\@<!\$>' contains=@shellcatShellScript

hi def link shellcatComment Comment

let b:current_syntax = "shellcat"

" vim:ts=4 sts=4 sw=4 et
