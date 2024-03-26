
#include "pressio_data.h"
#include "pressio_compressor.h"
#include "pressio_options.h"
#include "libpressio_ext/cpp/metrics.h"
#include "libpressio_ext/cpp/pressio.h"
#include "libpressio_ext/cpp/options.h"
#include "std_compat/memory.h"

#include <SZ3/predictor/Predictor.hpp>
#include "SZ3/predictor/LorenzoPredictor.hpp"
#include "SZ3/quantizer/Quantizer.hpp"
#include "SZ3/encoder/Encoder.hpp"
#include "SZ3/lossless/Lossless.hpp"
#include "SZ3/utils/Iterator.hpp"
#include "SZ3/utils/MemoryUtil.hpp"
#include "SZ3/utils/Config.hpp"
#include "SZ3/utils/FileUtil.hpp"
#include "SZ3/utils/Interpolators.hpp"
#include "SZ3/utils/Timer.hpp"
#include "SZ3/def.hpp"
#include "SZ3/utils/Config.hpp"
#include "SZ3/api/sz.hpp"
#include "SZ3/compressor/SZInterpolationCompressor.hpp"
#include "SZ3/compressor/deprecated/SZBlockInterpolationCompressor.hpp"
#include "SZ3/quantizer/IntegerQuantizer.hpp"
#include "SZ3/lossless/Lossless_zstd.hpp"
#include "SZ3/lossless/Lossless_bypass.hpp"
#include "SZ3/utils/Iterator.hpp"
#include "SZ3/utils/Statistic.hpp"
#include "SZ3/utils/Extraction.hpp"
#include "SZ3/utils/QuantOptimizatioin.hpp"
#include "SZ3/utils/Config.hpp"
#include "SZ3/api/impl/SZLorenzoReg.hpp"
#include "SZ3/utils/ByteUtil.hpp"
#include "SZ3/utils/ska_hash/unordered_map.hpp"
#include <memory>
#include <cstring>
#include <cmath>
#include <cfloat>
#include <fstream>
#include <sys/stat.h>
#include <limits.h>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <set>

namespace SZ3 {


    template<class T>
    class CustomHuffmanEncoder : public concepts::EncoderInterface<T> {

    public:

        typedef struct node_t {
            struct node_t *left, *right;
            size_t freq;
            char t; //in_node:0; otherwise:1
            T c;
        } *node;

        typedef struct HuffmanTree {
            unsigned int stateNum;
            unsigned int allNodes;
            struct node_t *pool;
            node *qqq, *qq; //the root node of the HuffmanTree is qq[1]
            int n_nodes; //n_nodes is for compression
            int qend;
            uint64_t **code;
            unsigned char *cout;
            int n_inode; //n_inode is for decompression
            int maxBitCount;
        } HuffmanTree;


        CustomHuffmanEncoder() {
            int x = 1;
            char *y = (char *) &x;
            if (*y == 1)
                sysEndianType = 0;
            else //=0
                sysEndianType = 1;
        }

        ~CustomHuffmanEncoder() {
            SZ_FreeHuffman();
        }

        //build huffman tree
        HuffmanTree *createHuffmanTree(int stateNum) {
            HuffmanTree *huffmanTree = (HuffmanTree *) malloc(sizeof(HuffmanTree));
            memset(huffmanTree, 0, sizeof(HuffmanTree));
            huffmanTree->stateNum = stateNum;
            huffmanTree->allNodes = 2 * stateNum;

            huffmanTree->pool = (struct node_t *) malloc(huffmanTree->allNodes * 2 * sizeof(struct node_t));
            huffmanTree->qqq = (node *) malloc(huffmanTree->allNodes * 2 * sizeof(node));
            huffmanTree->code = (uint64_t **) malloc(huffmanTree->stateNum * sizeof(uint64_t *));
            huffmanTree->cout = (unsigned char *) malloc(huffmanTree->stateNum * sizeof(unsigned char));

            memset(huffmanTree->pool, 0, huffmanTree->allNodes * 2 * sizeof(struct node_t));
            memset(huffmanTree->qqq, 0, huffmanTree->allNodes * 2 * sizeof(node));
            memset(huffmanTree->code, 0, huffmanTree->stateNum * sizeof(uint64_t *));
            memset(huffmanTree->cout, 0, huffmanTree->stateNum * sizeof(unsigned char));
            huffmanTree->qq = huffmanTree->qqq - 1;
            huffmanTree->n_nodes = 0;
            huffmanTree->n_inode = 0;
            huffmanTree->qend = 1;

            return huffmanTree;
        }

        /**
         * build huffman tree using bins
         * @param bins
         * @param stateNum is no longer needed
         */
        void preprocess_encode(const std::vector<T> &bins, int stateNum) {
            preprocess_encode(bins.data(), bins.size(), stateNum);
        }

        /**
         * build huffman tree using bins
         * @param bins
         * @param num_bin
         * @param stateNum is no longer needed
         */
        void preprocess_encode(const T *bins, size_t num_bin, int stateNum) {
            nodeCount = 0;
            if (num_bin == 0) {
                printf("Huffman bins should not be empty\n");
                exit(0);
            }
            init(bins, num_bin);
            for (int i = 0; i < huffmanTree->stateNum; i++)
                if (huffmanTree->code[i]) nodeCount++;
            nodeCount = nodeCount * 2 - 1;
        }

        //save the huffman Tree in the compressed data
        void save(uchar *&c) {
            auto cc = c;
            write(offset, c);
            int32ToBytes_bigEndian(c, nodeCount);
            c += sizeof(int);
            int32ToBytes_bigEndian(c, huffmanTree->stateNum / 2);
            c += sizeof(int);
            uint totalSize = 0;// = convert_HuffTree_to_bytes_anyStates(nodeCount, c);
            // std::cout << "nodeCount = " << nodeCount << std::endl;
            if (nodeCount <= 256)
                totalSize = convert_HuffTree_to_bytes_anyStates<unsigned char>(nodeCount, c);
            else if (nodeCount <= 65536)
                totalSize = convert_HuffTree_to_bytes_anyStates<unsigned short>(nodeCount, c);
            else
                totalSize = convert_HuffTree_to_bytes_anyStates<unsigned int>(nodeCount, c);
            c += totalSize;
        }

        size_t size_est() {
            size_t b = (nodeCount <= 256) ? sizeof(unsigned char) : ((nodeCount <= 65536) ? sizeof(unsigned short) : sizeof(unsigned int));
            return 1 + 2 * nodeCount * b + nodeCount * sizeof(unsigned char) + nodeCount * sizeof(T) + sizeof(int) + sizeof(int) + sizeof(T);
        }

        //perform encoding
        size_t encode(const std::vector<T> &bins, uchar *&bytes) {
            return encode(bins.data(), bins.size(), bytes);
        }

        uint64_t* get_code_for_state(T bin_index){
            int state = bin_index - offset;
            return (huffmanTree->code[state]);
        }

        unsigned char get_cout_for_state(T bin_index){
            int state = bin_index - offset;
            return (huffmanTree->cout[state]);
        }

        //perform encoding
        size_t encode(const T *bins, size_t num_bin, uchar *&bytes) {
            // printf("RAN ENCODE\n");
            size_t outSize = 0;
            size_t i = 0;
            unsigned char bitSize = 0, byteSize, byteSizep;
            int state;
            uchar *p = bytes + sizeof(size_t);
            int lackBits = 0;
            //int64_t totalBitSize = 0, maxBitSize = 0, bitSize21 = 0, bitSize32 = 0;
            for (i = 0; i < num_bin; i++) {
                state = bins[i] - offset;
                bitSize = huffmanTree->cout[state];

                if (lackBits == 0) {
                    byteSize = bitSize % 8 == 0 ? bitSize / 8 : bitSize / 8 +
                                                                1; //it's equal to the number of bytes involved (for *outSize)
                    byteSizep = bitSize / 8; //it's used to move the pointer p for next data
                    if (byteSize <= 8) {
                        int64ToBytes_bigEndian(p, (huffmanTree->code[state])[0]);
                        p += byteSizep;
                    } else //byteSize>8
                    {
                        int64ToBytes_bigEndian(p, (huffmanTree->code[state])[0]);
                        p += 8;
                        int64ToBytes_bigEndian(p, (huffmanTree->code[state])[1]);
                        p += (byteSizep - 8);
                    }
                    outSize += byteSize;
                    lackBits = bitSize % 8 == 0 ? 0 : 8 - bitSize % 8;
                } else {
                    *p = (*p) | (unsigned char) ((huffmanTree->code[state])[0] >> (64 - lackBits));
                    if (lackBits < bitSize) {
                        p++;

                        int64_t newCode = (huffmanTree->code[state])[0] << lackBits;
                        int64ToBytes_bigEndian(p, newCode);

                        if (bitSize <= 64) {
                            bitSize -= lackBits;
                            byteSize = bitSize % 8 == 0 ? bitSize / 8 : bitSize / 8 + 1;
                            byteSizep = bitSize / 8;
                            p += byteSizep;
                            outSize += byteSize;
                            lackBits = bitSize % 8 == 0 ? 0 : 8 - bitSize % 8;
                        } else //bitSize > 64
                        {
                            byteSizep = 7; //must be 7 bytes, because lackBits!=0
                            p += byteSizep;
                            outSize += byteSize;

                            bitSize -= 64;
                            if (lackBits < bitSize) {
                                *p = (*p) | (unsigned char) ((huffmanTree->code[state])[0] >> (64 - lackBits));
                                p++;
                                newCode = (huffmanTree->code[state])[1] << lackBits;
                                int64ToBytes_bigEndian(p, newCode);
                                bitSize -= lackBits;
                                byteSize = bitSize % 8 == 0 ? bitSize / 8 : bitSize / 8 + 1;
                                byteSizep = bitSize / 8;
                                p += byteSizep;
                                outSize += byteSize;
                                lackBits = bitSize % 8 == 0 ? 0 : 8 - bitSize % 8;
                            } else //lackBits >= bitSize
                            {
                                *p = (*p) | (unsigned char) ((huffmanTree->code[state])[0] >> (64 - bitSize));
                                lackBits -= bitSize;
                            }
                        }
                    } else //lackBits >= bitSize
                    {
                        lackBits -= bitSize;
                        if (lackBits == 0)
                            p++;
                    }
                }
            }
            *reinterpret_cast<size_t *>(bytes) = outSize;
            bytes += sizeof(size_t) + outSize;
            return outSize;
        }

        void postprocess_encode() {
            SZ_FreeHuffman();
        }

        void preprocess_decode() {};

        //perform decoding
        std::vector<T> decode(const uchar *&bytes, size_t targetLength) {
            node t = treeRoot;
            std::vector<T> out(targetLength);
            size_t i = 0, byteIndex = 0, count = 0;
            int r;
            node n = treeRoot;
            size_t encodedLength = *reinterpret_cast<const size_t *>(bytes);
            bytes += sizeof(size_t);
            if (n->t) //root->t==1 means that all state values are the same (constant)
            {
                for (count = 0; count < targetLength; count++)
                    out[count] = n->c + offset;
                return out;
            }

            for (i = 0; count < targetLength; i++) {
                byteIndex = i >> 3; //i/8
                r = i % 8;
                if (((bytes[byteIndex] >> (7 - r)) & 0x01) == 0)
                    n = n->left;
                else
                    n = n->right;

                if (n->t) {
                    out[count] = n->c + offset;
                    n = t;
                    count++;
                }
            }
            bytes += encodedLength;
            return out;
        }

        //empty function
        void postprocess_decode() {
            SZ_FreeHuffman();
        }

        //load Huffman tree
        void load(const uchar *&c, size_t &remaining_length) {
            read(offset, c, remaining_length);
            nodeCount = bytesToInt32_bigEndian(c);
            int stateNum = bytesToInt32_bigEndian(c + sizeof(int)) * 2;
            size_t encodeStartIndex;
            if (nodeCount <= 256)
                encodeStartIndex = 1 + 3 * nodeCount * sizeof(unsigned char) + nodeCount * sizeof(T);
            else if (nodeCount <= 65536)
                encodeStartIndex =
                        1 + 2 * nodeCount * sizeof(unsigned short) + nodeCount * sizeof(unsigned char) +
                        nodeCount * sizeof(T);
            else
                encodeStartIndex =
                        1 + 2 * nodeCount * sizeof(unsigned int) + nodeCount * sizeof(unsigned char) +
                        nodeCount * sizeof(T);

            huffmanTree = createHuffmanTree(stateNum);
            treeRoot = reconstruct_HuffTree_from_bytes_anyStates(c + sizeof(int) + sizeof(int), nodeCount);
            c += sizeof(int) + sizeof(int) + encodeStartIndex;
            loaded = true;
        }

        bool isLoaded() { return loaded; }

    private:
        HuffmanTree *huffmanTree = NULL;
        node treeRoot;
        unsigned int nodeCount = 0;
        uchar sysEndianType; //0: little endian, 1: big endian
        bool loaded = false;
        T offset;


        node reconstruct_HuffTree_from_bytes_anyStates(const unsigned char *bytes, uint nodeCount) {
            if (nodeCount <= 256) {
                unsigned char *L = (unsigned char *) malloc(nodeCount * sizeof(unsigned char));
                memset(L, 0, nodeCount * sizeof(unsigned char));
                unsigned char *R = (unsigned char *) malloc(nodeCount * sizeof(unsigned char));
                memset(R, 0, nodeCount * sizeof(unsigned char));
                T *C = (T *) malloc(nodeCount * sizeof(T));
                memset(C, 0, nodeCount * sizeof(T));
                unsigned char *t = (unsigned char *) malloc(nodeCount * sizeof(unsigned char));
                memset(t, 0, nodeCount * sizeof(unsigned char));
                // TODO: Endian type
                // unsigned char cmpSysEndianType = bytes[0];
                // if(cmpSysEndianType!=(unsigned char)sysEndianType)
                // {
                // 	unsigned char* p = (unsigned char*)(bytes+1+2*nodeCount*sizeof(unsigned char));
                // 	size_t i = 0, size = nodeCount*sizeof(unsigned int);
                // 	while(1)
                // 	{
                // 		symTransform_4bytes(p);
                // 		i+=sizeof(unsigned int);
                // 		if(i<size)
                // 			p+=sizeof(unsigned int);
                // 		else
                // 			break;
                // 	}
                // }
                memcpy(L, bytes + 1, nodeCount * sizeof(unsigned char));
                memcpy(R, bytes + 1 + nodeCount * sizeof(unsigned char), nodeCount * sizeof(unsigned char));
                memcpy(C, bytes + 1 + 2 * nodeCount * sizeof(unsigned char), nodeCount * sizeof(T));
                memcpy(t, bytes + 1 + 2 * nodeCount * sizeof(unsigned char) + nodeCount * sizeof(T),
                       nodeCount * sizeof(unsigned char));
                node root = this->new_node2(C[0], t[0]);
                this->unpad_tree<uchar>(L, R, C, t, 0, root);
                free(L);
                free(R);
                free(C);
                free(t);
                return root;
            } else if (nodeCount <= 65536) {
                unsigned short *L = (unsigned short *) malloc(nodeCount * sizeof(unsigned short));
                memset(L, 0, nodeCount * sizeof(unsigned short));
                unsigned short *R = (unsigned short *) malloc(nodeCount * sizeof(unsigned short));
                memset(R, 0, nodeCount * sizeof(unsigned short));
                T *C = (T *) malloc(nodeCount * sizeof(T));
                memset(C, 0, nodeCount * sizeof(T));
                unsigned char *t = (unsigned char *) malloc(nodeCount * sizeof(unsigned char));
                memset(t, 0, nodeCount * sizeof(unsigned char));

                // TODO: Endian type
                // unsigned char cmpSysEndianType = bytes[0];
                // if(cmpSysEndianType!=(unsigned char)sysEndianType)
                // {
                // 	unsigned char* p = (unsigned char*)(bytes+1);
                // 	size_t i = 0, size = 3*nodeCount*sizeof(unsigned int);
                // 	while(1)
                // 	{
                // 		symTransform_4bytes(p);
                // 		i+=sizeof(unsigned int);
                // 		if(i<size)
                // 			p+=sizeof(unsigned int);
                // 		else
                // 			break;
                // 	}
                // }

                memcpy(L, bytes + 1, nodeCount * sizeof(unsigned short));
                memcpy(R, bytes + 1 + nodeCount * sizeof(unsigned short), nodeCount * sizeof(unsigned short));
                memcpy(C, bytes + 1 + 2 * nodeCount * sizeof(unsigned short), nodeCount * sizeof(T));

                memcpy(t, bytes + 1 + 2 * nodeCount * sizeof(unsigned short) + nodeCount * sizeof(T),
                       nodeCount * sizeof(unsigned char));

                node root = this->new_node2(0, 0);
                this->unpad_tree<unsigned short>(L, R, C, t, 0, root);
                free(L);
                free(R);
                free(C);
                free(t);
                return root;
            } else //nodeCount>65536
            {
                unsigned int *L = (unsigned int *) malloc(nodeCount * sizeof(unsigned int));
                memset(L, 0, nodeCount * sizeof(unsigned int));
                unsigned int *R = (unsigned int *) malloc(nodeCount * sizeof(unsigned int));
                memset(R, 0, nodeCount * sizeof(unsigned int));
                T *C = (T *) malloc(nodeCount * sizeof(T));
                memset(C, 0, nodeCount * sizeof(T));
                unsigned char *t = (unsigned char *) malloc(nodeCount * sizeof(unsigned char));
                memset(t, 0, nodeCount * sizeof(unsigned char));
                // TODO: Endian type
                // unsigned char cmpSysEndianType = bytes[0];
                // if(cmpSysEndianType!=(unsigned char)sysEndianType)
                // {
                // 	unsigned char* p = (unsigned char*)(bytes+1);
                // 	size_t i = 0, size = 3*nodeCount*sizeof(unsigned int);
                // 	while(1)
                // 	{
                // 		symTransform_4bytes(p);
                // 		i+=sizeof(unsigned int);
                // 		if(i<size)
                // 			p+=sizeof(unsigned int);
                // 		else
                // 			break;
                // 	}
                // }

                memcpy(L, bytes + 1, nodeCount * sizeof(unsigned int));
                memcpy(R, bytes + 1 + nodeCount * sizeof(unsigned int), nodeCount * sizeof(unsigned int));
                memcpy(C, bytes + 1 + 2 * nodeCount * sizeof(unsigned int), nodeCount * sizeof(T));

                memcpy(t, bytes + 1 + 2 * nodeCount * sizeof(unsigned int) + nodeCount * sizeof(T),
                       nodeCount * sizeof(unsigned char));

                node root = this->new_node2(0, 0);
                this->unpad_tree<unsigned int>(L, R, C, t, 0, root);
                free(L);
                free(R);
                free(C);
                free(t);
                return root;
            }
        }

        node new_node(size_t freq, T c, node a, node b) {
            node n = huffmanTree->pool + huffmanTree->n_nodes++;
            if (freq) {
                n->c = c;
                n->freq = freq;
                n->t = 1;
            } else {
                n->left = a;
                n->right = b;
                n->freq = a->freq + b->freq;
                n->t = 0;
                //n->c = 0;
            }
            return n;
        }

        node new_node2(T c, unsigned char t) {
            huffmanTree->pool[huffmanTree->n_nodes].c = c;
            huffmanTree->pool[huffmanTree->n_nodes].t = t;
            return huffmanTree->pool + huffmanTree->n_nodes++;
        }

        /* priority queue */
        void qinsert(node n) {
            int j, i = huffmanTree->qend++;
            while ((j = (i >> 1)))  //j=i/2
            {
                if (huffmanTree->qq[j]->freq <= n->freq) break;
                huffmanTree->qq[i] = huffmanTree->qq[j], i = j;
            }
            huffmanTree->qq[i] = n;
        }

        node qremove() {
            int i, l;
            node n = huffmanTree->qq[i = 1];
            node p;
            if (huffmanTree->qend < 2) return 0;
            huffmanTree->qend--;
            huffmanTree->qq[i] = huffmanTree->qq[huffmanTree->qend];

            while ((l = (i << 1)) < huffmanTree->qend) {  //l=(i*2)
                if (l + 1 < huffmanTree->qend && huffmanTree->qq[l + 1]->freq < huffmanTree->qq[l]->freq) l++;
                if (huffmanTree->qq[i]->freq > huffmanTree->qq[l]->freq) {
                    p = huffmanTree->qq[i];
                    huffmanTree->qq[i] = huffmanTree->qq[l];
                    huffmanTree->qq[l] = p;
                    i = l;
                } else {
                    break;
                }
            }
            return n;
        }

        /* walk the tree and put 0s and 1s */
        /**
         * @out1 should be set to 0.
         * @out2 should be 0 as well.
         * @index: the index of the byte
         * */
        void build_code(node n, int len, uint64_t out1, uint64_t out2) {
            if (n->t) {
                huffmanTree->code[n->c] = (uint64_t *) malloc(2 * sizeof(uint64_t));
                if (len <= 64) {
                    (huffmanTree->code[n->c])[0] = out1 << (64 - len);
                    (huffmanTree->code[n->c])[1] = out2;
                } else {
                    (huffmanTree->code[n->c])[0] = out1;
                    (huffmanTree->code[n->c])[1] = out2 << (128 - len);
                }
                huffmanTree->cout[n->c] = (unsigned char) len;
                return;
            }
            int index = len >> 6; //=len/64
            if (index == 0) {
                out1 = out1 << 1;
                out1 = out1 | 0;
                build_code(n->left, len + 1, out1, 0);
                out1 = out1 | 1;
                build_code(n->right, len + 1, out1, 0);
            } else {
                if (len % 64 != 0)
                    out2 = out2 << 1;
                out2 = out2 | 0;
                build_code(n->left, len + 1, out1, out2);
                out2 = out2 | 1;
                build_code(n->right, len + 1, out1, out2);
            }
        }

        /**
         * Compute the frequency of the data and build the Huffman tree
         * @param HuffmanTree* huffmanTree (output)
         * @param int *s (input)
         * @param size_t length (input)
         * */
        void init(const T *s, size_t length) {
            T max = s[0];
            offset = s[0]; //offset is min

            ska::unordered_map<T, size_t> frequency;
            for (size_t i = 0; i < length; i++) {
                frequency[s[i]]++;
            }

            for (const auto &kv: frequency) {
                auto k = kv.first;
                if (k > max) {
                    max = k;
                }
                if (k < offset) {
                    offset = k;
                }
            }

            int stateNum = max - offset + 2;
            huffmanTree = createHuffmanTree(stateNum);

            for (const auto &f: frequency) {
                qinsert(new_node(f.second, f.first - offset, 0, 0));
            }

            while (huffmanTree->qend > 2)
                qinsert(new_node(0, 0, qremove(), qremove()));

            build_code(huffmanTree->qq[1], 0, 0, 0);
            treeRoot = huffmanTree->qq[1];

        }

        template<class T1>
        void pad_tree(T1 *L, T1 *R, T *C, unsigned char *t, unsigned int i, node root) {
            C[i] = root->c;
            t[i] = root->t;
            node lroot = root->left;
            if (lroot != 0) {
                huffmanTree->n_inode++;
                L[i] = huffmanTree->n_inode;
                pad_tree(L, R, C, t, huffmanTree->n_inode, lroot);
            }
            node rroot = root->right;
            if (rroot != 0) {
                huffmanTree->n_inode++;
                R[i] = huffmanTree->n_inode;
                pad_tree(L, R, C, t, huffmanTree->n_inode, rroot);
            }
        }

        template<class T1>
        void unpad_tree(T1 *L, T1 *R, T *C, unsigned char *t, unsigned int i, node root) {
            //root->c = C[i];
            if (root->t == 0) {
                T1 l, r;
                l = L[i];
                if (l != 0) {
                    node lroot = new_node2(C[l], t[l]);
                    root->left = lroot;
                    unpad_tree(L, R, C, t, l, lroot);
                }
                r = R[i];
                if (r != 0) {
                    node rroot = new_node2(C[r], t[r]);
                    root->right = rroot;
                    unpad_tree(L, R, C, t, r, rroot);
                }
            }
        }

        template<class T1>
        unsigned int convert_HuffTree_to_bytes_anyStates(unsigned int nodeCount, unsigned char *out) {
            T1 *L = (T1 *) malloc(nodeCount * sizeof(T1));
            memset(L, 0, nodeCount * sizeof(T1));
            T1 *R = (T1 *) malloc(nodeCount * sizeof(T1));
            memset(R, 0, nodeCount * sizeof(T1));
            T *C = (T *) malloc(nodeCount * sizeof(T));
            memset(C, 0, nodeCount * sizeof(T));
            unsigned char *t = (unsigned char *) malloc(nodeCount * sizeof(unsigned char));
            memset(t, 0, nodeCount * sizeof(unsigned char));

            pad_tree(L, R, C, t, 0, huffmanTree->qq[1]);

            unsigned int totalSize =
                    1 + 2 * nodeCount * sizeof(T1) + nodeCount * sizeof(unsigned char) + nodeCount * sizeof(T);
            //*out = (unsigned char*)malloc(totalSize);
            out[0] = (unsigned char) sysEndianType;
            memcpy(out + 1, L, nodeCount * sizeof(T1));
            memcpy(out + 1 + nodeCount * sizeof(T1), R, nodeCount * sizeof(T1));
            memcpy(out + 1 + 2 * nodeCount * sizeof(T1), C, nodeCount * sizeof(T));
            memcpy(out + 1 + 2 * nodeCount * sizeof(T1) + nodeCount * sizeof(T), t, nodeCount * sizeof(unsigned char));

            free(L);
            free(R);
            free(C);
            free(t);
            return totalSize;
        }

        void SZ_FreeHuffman() {
            if (huffmanTree != NULL) {
                size_t i;
                free(huffmanTree->pool);
                huffmanTree->pool = NULL;
                free(huffmanTree->qqq);
                huffmanTree->qqq = NULL;
                for (i = 0; i < huffmanTree->stateNum; i++) {
                    if (huffmanTree->code[i] != NULL)
                        free(huffmanTree->code[i]);
                }
                free(huffmanTree->code);
                huffmanTree->code = NULL;
                free(huffmanTree->cout);
                huffmanTree->cout = NULL;
                free(huffmanTree);
                huffmanTree = NULL;
            }
        }

    };
}


using namespace SZ3;

template<class T, uint N, class Quantizer, class Encoder, class Lossless>
class SZInterpolationEstimator {
public:
    SZInterpolationEstimator(Quantizer quantizer, Encoder encoder, Lossless lossless) :
            quantizer(quantizer), encoder(encoder), lossless(lossless) {

        static_assert(std::is_base_of<concepts::QuantizerInterface<T>, Quantizer>::value,
                      "must implement the quatizer interface");
        static_assert(std::is_base_of<concepts::EncoderInterface<int>, Encoder>::value,
                      "must implement the encoder interface");
        static_assert(std::is_base_of<concepts::LosslessInterface, Lossless>::value,
                      "must implement the lossless interface");
    }

    std::vector<int> get_quant_inds() {
            return quant_inds;
	}

    std::vector<double> estimate(const Config &conf, T *data, int input_stride) {


        Timer sample_timer(true);
        dimension_offsets[N - 1] = 1;
        for (int i = N - 2; i >= 0; i--) {
            dimension_offsets[i] = dimension_offsets[i + 1] * conf.dims[i + 1];
        }

        dimension_sequences = std::vector<std::array<int, N>>();
        auto sequence = std::array<int, N>();
        for (int i = 0; i < N; i++) {
            sequence[i] = i;
        }
        do {
            dimension_sequences.push_back(sequence);
        } while (std::next_permutation(sequence.begin(), sequence.end()));


        std::array<size_t, N> begin_idx, end_idx;
        for (int i = 0; i < N; i++) {
            begin_idx[i] = 0;
            end_idx[i] = conf.dims[i] - 1;
        }

        int interpolation_level = -1;
        for (int i = 0; i < N; i++) {
            if (interpolation_level < ceil(log2(conf.dims[i]))) {
                interpolation_level = (int) ceil(log2(conf.dims[i]));
            }
        }

        {
            // This code block collects compression errors from a higher level into cmpr_err
            // it is observed that most levels have similar error distribution
            sample_stride = 1;
            cmpr_err.clear();
            cmpr_err.reserve(1000);
            for (uint level = interpolation_level; level > 1 && level <= interpolation_level; level--) {
                size_t stride = 1U << (level - 1);
                block_interpolation(data, begin_idx, end_idx, PB_predict_collect_err,
                                    interpolators[conf.interpAlgo], conf.interpDirection, stride);
                if (cmpr_err.size() > 1000) {
                    break;
                }
                cmpr_err.clear();
            }
        }

        {
            // the sampling process is done only on the last level, because it covers 87.5% of data points for 3D (and 75% for 2D).
            // sample_stride controls the distance of the data points covered in the sampling process.
            // original data points are used during sampling, to simulate the error impact/to make them as decompressed data,
            // errors are randomly select from cmpr_err and added for interpolation calculation.
            // TODO
            // Because sample_stride is used, the sampled quant_inds may not have same CR as the original quant_inds.
            // One possible solution is to add some zeros manually to simulate the original quant_inds
            gen = std::mt19937(rd());
            dist = std::uniform_int_distribution<>(0, cmpr_err.size());
            // sample_stride = 5;
            sample_stride = input_stride;
            // sample_stride = 2;
            quant_inds.clear();
            quant_inds.reserve(conf.num);
            block_interpolation(data, begin_idx, end_idx, PB_predict,
                                interpolators[conf.interpAlgo], conf.interpDirection, 1);
        }

        double sampling_dur = sample_timer.stop("sampling");

        encoder.preprocess_encode(quant_inds, 0);
        size_t bufferSize = 1.2 * (quantizer.size_est() + encoder.size_est() + sizeof(T) * quant_inds.size());

        uchar *buffer = new uchar[bufferSize];
        uchar *buffer_pos = buffer;

        quantizer.save(buffer_pos);
        quantizer.postcompress_data();
        const uchar * pos = buffer_pos;

        encoder.save(buffer_pos);
        const uchar* temp = buffer_pos;
        size_t tree_size = buffer_pos - pos;
        
        // size_t huff_size = encoder.encode(quant_inds, buffer_pos);
        // auto encoder2 = SZ3::HuffmanEncoder<int>();
        // encoder2.load(pos, tree_size);
        // auto decoded_data = encoder2.decode(temp, quant_inds.size());

        // int count = 0;
        // for(int i = 0; i < decoded_data.size(); i++){
        //     if(quant_inds[i] != decoded_data[i]){
        //         count += 1;
        //     }
        // }

        // printf("COUNT: %i\n", count);
        // // printf("Unpred Huff Code: %i, %i\nZero Huff Code: %i, %i\n", encoder.get_code_for_state(0)[0], encoder.get_code_for_state(0)[1], encoder.get_code_for_state(conf.quantbinCnt / 2 )[0], encoder.get_code_for_state(conf.quantbinCnt / 2)[1]);
        // printf("Unpred Huff Code: %i\nZero Huff Code: %i\n", encoder.get_cout_for_state(0), encoder.get_cout_for_state(conf.quantbinCnt / 2));

        std::unordered_map<int,int> freq;
        int num_samples = quant_inds.size();
        int zero_bin_index = conf.quantbinCnt / 2;
        int zero_ind_freq = 0;
        int radius = 10;
        int zr_ind = (radius/2) -1; 
        int freqs[radius] = {0};
        for(int i = 0; i < num_samples; i++){
            if(quant_inds[i] == zero_bin_index){
                zero_ind_freq += 1;
                freqs[zr_ind] += 1;
            }
            else if(abs(quant_inds[i] - zero_bin_index) <= radius/2 ){
                int index;
                if(quant_inds[i] > zero_bin_index){
                    index = quant_inds[i] - zero_bin_index + zr_ind;
                }
                else if(quant_inds[i] < zero_bin_index){
                    index = zr_ind - (zero_bin_index - quant_inds[i]);
                }
                freqs[index] += 1;
            }
            freq[quant_inds[i]]++;
        }

        int center_freq = 0;
        for(int i = 0; i < radius; i++){
            int ind = i - zr_ind + zero_bin_index;
            // printf("Bin [%i]: %i | %f - %i\n", i, freqs[i], (float)freqs[i]/num_samples, encoder.get_cout_for_state(ind));
            center_freq += freqs[i];
        }

        int zero_code_len = encoder.get_cout_for_state(zero_bin_index);

        // printf("Central Bins Freq: %i | %f\n", center_freq, center_freq * 1.0 / quant_inds.size());

        std::vector<std::pair<int, int>> keyFrequencyPairs;
        int k = 5;

        // Copy key-value pairs from the unordered map to the vector
        for (const auto& kv : freq) {
            keyFrequencyPairs.push_back(std::make_pair(kv.first, kv.second));
        }
        std::sort(keyFrequencyPairs.begin(), keyFrequencyPairs.end(),
              [](const std::pair<int, int>& p1, const std::pair<int, int>& p2) {
                  return p1.second > p2.second;
              });

        std::vector<int> topk_quantcodes;
        std::vector<int> topk_codelens;

        // std::cout << "Top " << k << " most frequent keys and their frequencies:" << std::endl;
        for (int i = 0; i < k && i < keyFrequencyPairs.size(); ++i) {
            // std::cout << "Key: " << keyFrequencyPairs[i].first << ", Frequency: "
                    // << keyFrequencyPairs[i].second << ", " << keyFrequencyPairs[i].second * 1.0 / num_samples
                    // << ", CodeLen: " << std::to_string(encoder.get_cout_for_state(keyFrequencyPairs[i].first)) << std::endl;

            // dont duplicate zero code estimate
            if(keyFrequencyPairs[i].first != conf.quantbinCnt /2){
                topk_quantcodes.push_back(keyFrequencyPairs[i].first);
                topk_codelens.push_back(encoder.get_cout_for_state(keyFrequencyPairs[i].first));
            }

        }

        encoder.postprocess_encode();
//            timer.stop("Coding");
        assert(buffer_pos - buffer < bufferSize);

        // printf("Estimator huffman outsize %i\n", huff_size);
		size_t postHuffmanBuffSize = buffer_pos - buffer;

        size_t compressed_size = 0;
        uchar *lossless_data = lossless.compress(buffer,
                                                 buffer_pos - buffer,
                                                 compressed_size);
        lossless.postcompress_data(buffer);

        // printf("Precompressedsize %i\n", compressed_size);

		// printf("Estimator huffman buffer length: %i\n", postHuffmanBuffSize);

        double prediction = 0.0;
        for (int i = 1; i < conf.quantbinCnt; i++) {
            if (freq[i] != 0) {
                float temp_bit = -log2((float)freq[i]/num_samples);
                //printf("%f %d\n", temp_bit, i);
                if (temp_bit < 32) {
                    if (temp_bit < 1) {
//                            printf("layer: %d %f\n", i, temp_bit);
                        if (i == zero_bin_index) prediction += ((float)freq[i]/num_samples) * 1;
                        else if (i == zero_bin_index-1) prediction += ((float)freq[i]/num_samples) * 2.5;
                        else if (i == zero_bin_index+1) prediction += ((float)freq[i]/num_samples) * 2.5;
                        else prediction += ((float)freq[i]/num_samples) * 4;
                    }
                    else
                        prediction += ((float)freq[i]/num_samples) * temp_bit;
                }
            }
        }
        if (freq[0] != 0) 
            prediction += ((float)freq[0]/num_samples) * 32;
            // account for uncompressed values
            compressed_size += freq[0] * sizeof(T);
            // printf("qind[0]: %i, addition: %i, %f\n", freq[0], freq[0]*sizeof(T), ((float)freq[0]/num_samples) * 32);


        float p_0 = (float) zero_ind_freq / num_samples; // percent quant inds that are zero
        float P_0; // percent of post huffman buffer size that is made up of the symbol corresponding to the zero code
        if(zero_ind_freq > num_samples/2){
            P_0 = p_0 * 1.0 / prediction;//postHuffmanBuffSize;
        }
        else {
            P_0 = -(((float)p_0) * log2((float)p_0))/ prediction;//postHuffmanBuffSize;
        }

        // printf("EST -- Zero bin: %i, Zero freq: %i, n_samples: %i, p_0: %f\n", zero_bin_index, zero_ind_freq, num_samples, p_0);

        // printf("Postcompressedsize %i\n", compressed_size);
        float pred = 1 / ((1 - p_0) * P_0 + (1 - P_0));
        float reduction_efficiency = ((1 - p_0) * P_0);
        float sum_probs = P_0;
        // printf("ratiopred: %f\n", ratio);
        // printf("compressed_size: %i\n", compressed_size);
        // printf("PredEstBuffSize: %f, PredEstHuffmanCR: %f\n", prediction * quant_inds.size(), quant_inds.size() * sizeof(T) * 1.0 / prediction);

        float huffpred = quant_inds.size() * sizeof(T) * 8.0 / (prediction * quant_inds.size());
        float lossless_pred = 32 / (prediction / pred);

        // compile reduction estimates for top K bins
        float pred_reduction_efficiency = reduction_efficiency;
        for(int i = 0; i < topk_quantcodes.size(); i++){
            int code = topk_quantcodes[i];
            int codelen = topk_codelens[i];
            // percent frequency
            float p_si = freq[code] * 1.0 / num_samples;
            // percent of space in huffman buffer taken by code
            float P_si = -(((float)p_si) * log2((float)p_si))/ prediction;
            // to compare empirical codelen estimation to theoretical codelen above
            float emp_P_si = ((float)p_si) * ((float)codelen) / prediction;

            pred_reduction_efficiency += ((1 - p_si) * P_si);
            sum_probs += P_si;
        }

        float pred_red_ratio = 1 / (pred_reduction_efficiency + (1-sum_probs));
        float lossless_red_ratio = 32 / (prediction / pred_red_ratio);

        // printf("newsize: %f\n", new_size);
        // printf("huffcr_pred: %f, lossless_pred: %f\n", huffpred, lossless_pred);
        // printf("lossless pred: %f, pred_red_ratio: %f\n", lossless_pred, lossless_red_ratio);

        return {huffpred, lossless_pred, sampling_dur};//lossless_pred; //32.0 / prediction;

	}
private:

    enum PredictorBehavior {
        PB_predict_collect_err, PB_predict, PB_recover
    };

    std::uniform_int_distribution<> dist;
    std::random_device rd;
    std::mt19937 gen;

    std::vector<T> cmpr_err;
    int sample_stride = 5;

    //quantize and record the quantization bins
    inline void quantize(size_t idx, T &d, T pred) {
        quant_inds.push_back(quantizer.quantize(d, pred));
    }


    //quantize and record compression error
    inline void quantize2(size_t idx, T &d, T pred) {
        T d0 = d;
        quantizer.quantize_and_overwrite(d, pred);
        cmpr_err.push_back(d0 - d);
        d = d0;
    }

    //Add noise/compression error to original data, to simulate it as decompressed data
    inline T s(T d) {
        return d + cmpr_err[dist(gen)];
    }

    double block_interpolation_1d(T *data, size_t begin, size_t end, size_t stride,
                                  const std::string &interp_func,
                                  const PredictorBehavior pb) {
        size_t n = (end - begin) / stride + 1;
        if (n <= 1) {
            return 0;
        }

        size_t stride3x = 3 * stride;
        size_t stride5x = 5 * stride;

        if (pb == PB_predict_collect_err) {
            if (interp_func == "linear" || n < 5) {
                for (size_t i = 1; i + 1 < n; i += 2 * sample_stride) {
                    T *d = data + begin + i * stride;
                    quantize2(d - data, *d, interp_linear(*(d - stride), *(d + stride)));
                }
                if (n % 2 == 0) {
                    T *d = data + begin + (n - 1) * stride;
                    if (n < 4) {
                        quantize2(d - data, *d, *(d - stride));
                    } else {
                        quantize2(d - data, *d, interp_linear1(*(d - stride3x), *(d - stride)));
                    }
                }
            } else {
                T *d;
                size_t i;
                for (i = 3; i + 3 < n; i += 2 * sample_stride) {
                    d = data + begin + i * stride;
                    quantize2(d - data, *d,
                                               interp_cubic(*(d - stride3x), *(d - stride), *(d + stride), *(d + stride3x)));
                }
                d = data + begin + stride;
                quantize2(d - data, *d, interp_quad_1(*(d - stride), *(d + stride), *(d + stride3x)));


                d = data + begin + ((n % 2 == 0) ? n - 3 : n - 2) * stride;
                quantize2(d - data, *d, interp_quad_2(*(d - stride3x), *(d - stride), *(d + stride)));
                if (n % 2 == 0) {
                    d = data + begin + (n - 1) * stride;
                    quantize2(d - data, *d, interp_quad_3(*(d - stride5x), *(d - stride3x), *(d - stride)));
                }

            }
        } else {
            if (interp_func == "linear" || n < 5) {
                for (size_t i = 1; i + 1 < n; i += 2 * sample_stride) {
                    T *d = data + begin + i * stride;
                    quantize(d - data, *d, interp_linear(s(*(d - stride)), s(*(d + stride))));
                }
                if (n % 2 == 0) {
                    T *d = data + begin + (n - 1) * stride;
                    if (n < 4) {
                        quantize(d - data, *d, s(*(d - stride)));
                    } else {
                        quantize(d - data, *d, interp_linear1(s(*(d - stride3x)), s(*(d - stride))));
                    }
                }
            } else {
                T *d;
                size_t i;
                for (i = 3; i + 3 < n; i += 2 * sample_stride) {
                    d = data + begin + i * stride;
                    quantize(d - data, *d,
                                              interp_cubic(s(*(d - stride3x)), s(*(d - stride)), s(*(d + stride)), s(*(d + stride3x))));
                }
                d = data + begin + stride;
                quantize(d - data, *d, interp_quad_1(s(*(d - stride)), s(*(d + stride)), s(*(d + stride3x))));


                d = data + begin + ((n % 2 == 0) ? n - 3 : n - 2) * stride;
                quantize(d - data, *d, interp_quad_2(s(*(d - stride3x)), s(*(d - stride)), s(*(d + stride))));
                if (n % 2 == 0) {
                    d = data + begin + (n - 1) * stride;
                    quantize(d - data, *d, interp_quad_3(s(*(d - stride5x)), s(*(d - stride3x)), s(*(d - stride))));
                }
            }
        }

        return 0;
    }

    template<uint NN = N>
    typename std::enable_if<NN == 1, double>::type
    block_interpolation(T *data, std::array<size_t, N> begin, std::array<size_t, N> end, const PredictorBehavior pb,
                        const std::string &interp_func, const int direction, size_t stride = 1) {
        return block_interpolation_1d(data, begin[0], end[0], stride, interp_func, pb);
    }

    template<uint NN = N>
    typename std::enable_if<NN == 2, double>::type
    block_interpolation(T *data, std::array<size_t, N> begin, std::array<size_t, N> end, const PredictorBehavior pb,
                        const std::string &interp_func, const int direction, size_t stride = 1) {
        double predict_error = 0;
        size_t stride2x = stride * 2;
        const std::array<int, N> dims = dimension_sequences[direction];
        for (size_t j = (begin[dims[1]] ? begin[dims[1]] + stride2x : 0); j <= end[dims[1]]; j += stride2x * sample_stride) {
            size_t begin_offset = begin[dims[0]] * dimension_offsets[dims[0]] + j * dimension_offsets[dims[1]];
            predict_error += block_interpolation_1d(data, begin_offset,
                                                    begin_offset +
                                                    (end[dims[0]] - begin[dims[0]]) * dimension_offsets[dims[0]],
                                                    stride * dimension_offsets[dims[0]], interp_func, pb);
        }
        for (size_t i = (begin[dims[0]] ? begin[dims[0]] + stride : 0); i <= end[dims[0]]; i += stride * sample_stride) {
            size_t begin_offset = i * dimension_offsets[dims[0]] + begin[dims[1]] * dimension_offsets[dims[1]];
            predict_error += block_interpolation_1d(data, begin_offset,
                                                    begin_offset +
                                                    (end[dims[1]] - begin[dims[1]]) * dimension_offsets[dims[1]],
                                                    stride * dimension_offsets[dims[1]], interp_func, pb);
        }
        return predict_error;
    }

    template<uint NN = N>
    typename std::enable_if<NN == 3, double>::type
    block_interpolation(T *data, std::array<size_t, N> begin, std::array<size_t, N> end, const PredictorBehavior pb,
                        const std::string &interp_func, const int direction, size_t stride = 1) {
        double predict_error = 0;
        size_t stride2x = stride * 2;
        const std::array<int, N> dims = dimension_sequences[direction];
        for (size_t j = (begin[dims[1]] ? begin[dims[1]] + stride2x : 0); j <= end[dims[1]]; j += stride2x * sample_stride) {
            for (size_t k = (begin[dims[2]] ? begin[dims[2]] + stride2x : 0); k <= end[dims[2]]; k += stride2x * sample_stride) {
                size_t begin_offset = begin[dims[0]] * dimension_offsets[dims[0]] + j * dimension_offsets[dims[1]] +
                                      k * dimension_offsets[dims[2]];
                predict_error += block_interpolation_1d(data, begin_offset,
                                                        begin_offset +
                                                        (end[dims[0]] - begin[dims[0]]) *
                                                        dimension_offsets[dims[0]],
                                                        stride * dimension_offsets[dims[0]], interp_func, pb);
            }
        }
        for (size_t i = (begin[dims[0]] ? begin[dims[0]] + stride : 0); i <= end[dims[0]]; i += stride * sample_stride) {
            for (size_t k = (begin[dims[2]] ? begin[dims[2]] + stride2x : 0); k <= end[dims[2]]; k += stride2x * sample_stride) {
                size_t begin_offset = i * dimension_offsets[dims[0]] + begin[dims[1]] * dimension_offsets[dims[1]] +
                                      k * dimension_offsets[dims[2]];
                predict_error += block_interpolation_1d(data, begin_offset,
                                                        begin_offset +
                                                        (end[dims[1]] - begin[dims[1]]) *
                                                        dimension_offsets[dims[1]],
                                                        stride * dimension_offsets[dims[1]], interp_func, pb);
            }
        }
        for (size_t i = (begin[dims[0]] ? begin[dims[0]] + stride : 0); i <= end[dims[0]]; i += stride * sample_stride) {
            for (size_t j = (begin[dims[1]] ? begin[dims[1]] + stride : 0); j <= end[dims[1]]; j += stride * sample_stride) {
                size_t begin_offset = i * dimension_offsets[dims[0]] + j * dimension_offsets[dims[1]] +
                                      begin[dims[2]] * dimension_offsets[dims[2]];
                predict_error += block_interpolation_1d(data, begin_offset,
                                                        begin_offset +
                                                        (end[dims[2]] - begin[dims[2]]) *
                                                        dimension_offsets[dims[2]],
                                                        stride * dimension_offsets[dims[2]], interp_func, pb);
            }
        }
        return predict_error;
    }


    template<uint NN = N>
    typename std::enable_if<NN == 4, double>::type
    block_interpolation(T *data, std::array<size_t, N> begin, std::array<size_t, N> end, const PredictorBehavior pb,
                        const std::string &interp_func, const int direction, size_t stride = 1) {
        double predict_error = 0;
        size_t stride2x = stride * 2;
        const std::array<int, N> dims = dimension_sequences[direction];
        for (size_t j = (begin[dims[1]] ? begin[dims[1]] + stride2x : 0); j <= end[dims[1]]; j += stride2x * sample_stride) {
            for (size_t k = (begin[dims[2]] ? begin[dims[2]] + stride2x : 0); k <= end[dims[2]]; k += stride2x * sample_stride) {
                for (size_t t = (begin[dims[3]] ? begin[dims[3]] + stride2x : 0);
                     t <= end[dims[3]]; t += stride2x * sample_stride) {
                    size_t begin_offset =
                            begin[dims[0]] * dimension_offsets[dims[0]] + j * dimension_offsets[dims[1]] +
                            k * dimension_offsets[dims[2]] +
                            t * dimension_offsets[dims[3]];
                    predict_error += block_interpolation_1d(data, begin_offset,
                                                            begin_offset +
                                                            (end[dims[0]] - begin[dims[0]]) *
                                                            dimension_offsets[dims[0]],
                                                            stride * dimension_offsets[dims[0]], interp_func, pb);
                }
            }
        }
        for (size_t i = (begin[dims[0]] ? begin[dims[0]] + stride : 0); i <= end[dims[0]]; i += stride * sample_stride) {
            for (size_t k = (begin[dims[2]] ? begin[dims[2]] + stride2x : 0); k <= end[dims[2]]; k += stride2x * sample_stride) {
                for (size_t t = (begin[dims[3]] ? begin[dims[3]] + stride2x : 0);
                     t <= end[dims[3]]; t += stride2x * sample_stride) {
                    size_t begin_offset =
                            i * dimension_offsets[dims[0]] + begin[dims[1]] * dimension_offsets[dims[1]] +
                            k * dimension_offsets[dims[2]] +
                            t * dimension_offsets[dims[3]];
                    predict_error += block_interpolation_1d(data, begin_offset,
                                                            begin_offset +
                                                            (end[dims[1]] - begin[dims[1]]) *
                                                            dimension_offsets[dims[1]],
                                                            stride * dimension_offsets[dims[1]], interp_func, pb);
                }
            }
        }
        for (size_t i = (begin[dims[0]] ? begin[dims[0]] + stride : 0); i <= end[dims[0]]; i += stride * sample_stride) {
            for (size_t j = (begin[dims[1]] ? begin[dims[1]] + stride : 0); j <= end[dims[1]]; j += stride * sample_stride) {
                for (size_t t = (begin[dims[3]] ? begin[dims[3]] + stride2x : 0);
                     t <= end[dims[3]]; t += stride2x * sample_stride) {
                    size_t begin_offset = i * dimension_offsets[dims[0]] + j * dimension_offsets[dims[1]] +
                                          begin[dims[2]] * dimension_offsets[dims[2]] +
                                          t * dimension_offsets[dims[3]];
                    predict_error += block_interpolation_1d(data, begin_offset,
                                                            begin_offset +
                                                            (end[dims[2]] - begin[dims[2]]) *
                                                            dimension_offsets[dims[2]],
                                                            stride * dimension_offsets[dims[2]], interp_func, pb);
                }
            }
        }

        for (size_t i = (begin[dims[0]] ? begin[dims[0]] + stride : 0); i <= end[dims[0]]; i += stride * sample_stride) {
            for (size_t j = (begin[dims[1]] ? begin[dims[1]] + stride : 0); j <= end[dims[1]]; j += stride * sample_stride) {
                for (size_t k = (begin[dims[2]] ? begin[dims[2]] + stride : 0); k <= end[dims[2]]; k += stride * sample_stride) {
                    size_t begin_offset =
                            i * dimension_offsets[dims[0]] + j * dimension_offsets[dims[1]] +
                            k * dimension_offsets[dims[2]] +
                            begin[dims[3]] * dimension_offsets[dims[3]];
                    predict_error += block_interpolation_1d(data, begin_offset,
                                                            begin_offset +
                                                            (end[dims[3]] - begin[dims[3]]) *
                                                            dimension_offsets[dims[3]],
                                                            stride * dimension_offsets[dims[3]], interp_func, pb);
                }
            }
        }
        return predict_error;
    }

public:
    std::vector<std::string> interpolators = {"linear", "cubic"};
    std::array<size_t, N> dimension_offsets;
    std::vector<std::array<int, N>> dimension_sequences;
    Quantizer quantizer;
    Encoder encoder;
    Lossless lossless;
    std::vector<int> quant_inds;
};

/*
This code is only good for estimating the CR post huffman coding, the lossless predictions break down for larger error bounds
(We focus on the huffman CR in the paper)
*/
template<uint N, typename T>
double estimate_compress(Config conf, T *data, double abs, int stride) {
    conf.cmprAlgo = ALGO_INTERP;
    conf.interpAlgo = INTERP_ALGO_CUBIC;
    conf.interpDirection = 0;
    conf.errorBoundMode = SZ3::EB_ABS;
    conf.absErrorBound = abs;

	int numBins = conf.quantbinCnt / 2;

  SZInterpolationEstimator<T, N, SZ3::LinearQuantizer<T>, SZ3::CustomHuffmanEncoder<int>, SZ3::Lossless_bypass> estimator(
          SZ3::LinearQuantizer<T>(conf.absErrorBound, conf.quantbinCnt / 2),
          SZ3::CustomHuffmanEncoder<int>(),
          SZ3::Lossless_bypass());
  std::vector<double> estresults = estimator.estimate(conf, data, stride);
  double estCR = estresults[0];
  double sample_dur = estresults[2];

  return estCR;

}

namespace libpressio { namespace khan2023_sz3_metrics_ns {

class khan2023_sz3_plugin : public libpressio_metrics_plugin {
  public:
    int begin_compress_impl(struct pressio_data const* input, pressio_data const*) override {
      assert(input->dtype() == pressio_float_dtype);
      assert(input->num_dimensions() < 5);
      float * data = static_cast<float*>(input->data());
      auto dimensions = input->dimensions();
      switch(input->num_dimensions()) {
        case 1:
        {
          SZ3::Config conf(dimensions[0]);
          estimate = estimate_compress<1>(conf, data, abs_bound, stride);
          break;
        }
        case 2:
        {
          SZ3::Config conf(dimensions[0], dimensions[1]);
          estimate = estimate_compress<2>(conf, data, abs_bound, stride);
          break;
        }
        case 3:
        {
          SZ3::Config conf(dimensions[0], dimensions[1], dimensions[2]);
          estimate = estimate_compress<3>(conf, data, abs_bound, stride);
          break;
        }
        case 4:
        {
          SZ3::Config conf(dimensions[0], dimensions[1], dimensions[2], dimensions[3]);
          estimate = estimate_compress<4>(conf, data, abs_bound, stride);
          break;
        }
      }
      

      return 0;
    }

  
  struct pressio_options get_options() const override {
    pressio_options opts;
    set(opts, "khan2023_sz3:stride", stride);
    set(opts, "pressio:abs", abs_bound);
    return opts;
  }
  int set_options(pressio_options const& opts) override {
    get(opts, "khan2023_sz3:stride", &stride);
    get(opts, "pressio:abs", &abs_bound);


    return 0;
  }

  struct pressio_options get_configuration_impl() const override {
    pressio_options opts;
    set(opts, "predictors:invalidate", std::vector<std::string>{"predictors:error_dependent"});
    set(opts, "predictors:requires_decompress", false);
    set(opts, "pressio:stability", "stable");
    set(opts, "pressio:thread_safe", pressio_thread_safety_multiple);
    return opts;
  }

  struct pressio_options get_documentation_impl() const override {
    pressio_options opt;
    set(opt, "pressio:description", "predicts the compression ratio following huffman coding for the SZ3 interpolation predictor");
    set(opt, "khan2023_sz3:stride", "initial stride for the interpolation predictor, recommended: [2,5,10,15]");
    set(opt, "khan2023_sz3:abs", "absolute error bound to predict for");
    set(opt, "pressio:abs", "absolute error bound to predict for");
    set(opt, "khan2023_sz3:size:compression_ratio", "the predicted compression ratio");

    return opt;
  }

  pressio_options get_metrics_results(pressio_options const &) override {
    pressio_options opt;
    set(opt, "khan2023_sz3:size:compression_ratio", estimate);
    return opt;
  }

  std::unique_ptr<libpressio_metrics_plugin> clone() override {
    return compat::make_unique<khan2023_sz3_plugin>(*this);
  }
  const char* prefix() const override {
    return "khan2023_sz3";
  }

  private:
    int stride = 5;
    double abs_bound = 0.01;
    compat::optional<double> estimate;
};

static pressio_register metrics_khan2023_sz3_plugin(metrics_plugins(), "khan2023_sz3", [](){ return compat::make_unique<khan2023_sz3_plugin>(); });
}}

