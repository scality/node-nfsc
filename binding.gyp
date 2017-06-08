{
    "targets": [
        {
            "target_name": "node-nfsc",
            "sources": [
                "<!@(rpc/genrpc.sh rpc/mount3.x)",
                "<!@(rpc/genrpc.sh rpc/nfs3.x)",
                "rpc/mount3_clnt.c",
                "rpc/nfs3_clnt.c",
                "rpc/mount3_xdr.c",
                "rpc/nfs3_xdr.c",
                "src/node_nfsc.cc",
                "src/node_nfsc_errors3.cc",
                "src/node_nfsc_fattr3.cc",
                "src/node_nfsc_sattr3.cc",
                "src/node_nfsc_wcc3.cc",
                "src/node_nfsc_null3.cc",
                "src/node_nfsc_mount3.cc",
                "src/node_nfsc_unmount3.cc",
                "src/node_nfsc_lookup3.cc",
                "src/node_nfsc_getattr3.cc",
                "src/node_nfsc_readdir3.cc",
                "src/node_nfsc_readdirplus3.cc",
                "src/node_nfsc_access3.cc",
                "src/node_nfsc_read3.cc",
                "src/node_nfsc_write3.cc",
                "src/node_nfsc_commit3.cc",
                "src/node_nfsc_create3.cc",
                "src/node_nfsc_remove3.cc",
                "src/node_nfsc_rmdir3.cc",
                "src/node_nfsc_mkdir3.cc",
                "src/node_nfsc_setattr3.cc",
                "src/node_nfsc_rename3.cc",
                "src/node_nfsc_mknod3.cc",
                "src/node_nfsc_symlink3.cc",
            ],
            "cflags": [
                "-Wno-missing-field-initializers",
                "-Wno-unused-variable",
                "<!(pkg-config gssrpc --cflags)>",
                "-ggdb3"
            ],
            "ldflags": [
                "<!(pkg-config gssrpc --libs-only-L --libs-only-other)"
            ],
            "include_dirs": [
                "<(module_root_dir)/include",
                "<(module_root_dir)/rpc",
                "<!(node -e \"require('nan')\")"
            ],
            "libraries": [
                "<!(pkg-config gssrpc --libs-only-l)"
            ]
        }
    ]
}
