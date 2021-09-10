typedef unsigned int GLuint;
typedef char GLchar;
#define BLIT_TEXTURE_NB_PROG (16*2*4*14*5*4)
#define PG_MAX (BLIT_TEXTURE_NB_PROG+1024)
#define QuoteIdent(ident) #ident
#define Stringify(macro) QuoteIdent(macro)
#define SHADER_VERSION "#version 330 core \n"
#define SHADER_VERSION_TESS "#version 420 core \n"

#undef NULL
#define NULL nullptr

#define SHADER_GEN_INCLUDED
#include "opengl/common/src/common_glshader.c"
#include "opengl/common/src/ogl_shader.c"
#undef SHADER_GEN_INCLUDED

#include <fstream>
#include <sstream>
#include <vector>
#include <execution>
#include <algorithm>
#include <filesystem>
#include <unordered_set>
#include <iostream>

int main(int argc, char** argv)
{
    std::vector<std::tuple<int, std::string>> shaders;
    std::unordered_set<std::string> unique_shaders;

    //VDP2 programs
    for (int j = 0; j < 4; j++) {
        // 4 Sprite color calculation mode
        for (int k = 0; k < 2; k++) {
            // Palette only mode or palette/RGB mode
            for (int l = 0; l < 16; l++) {
                //16 sprite typed
                for (int m = 0; m < 14; m++) {
                    //14 screens configuration
                    for (int i = 0; i < 5; i++) {
                        // Sprite color calculation condition are separated by 1
                        for (int n = 0; n < 4; n++) {
                            //4 possibilities betwwen vdp2 and vdp1 FB width mode
                            int const index = 4 * (5 * (14 * (16 * (2 * j + k) + l) + m) + i) + n;

                            std::ostringstream shader;

                            shader << vdp2blit_gl_start_f[m % 7];
                            shader << Yglprg_vdp2_common_start;
                            shader << vdp2blit_palette_mode_f[k];
                            shader << vdp2blit_sprite_code_f[n];
                            shader << vdp2blit_srite_type_f[l];
                            shader << Yglprg_vdp2_drawfb_gl_cram_f;
                            shader << Yglprg_vdp2_common_draw;
                            shader << Yglprg_color_condition_f[i];
                            shader << Yglprg_color_mode_f[j];
                            shader << Yglprg_vdp2_drawfb_cram_eiploge_f;
                            shader << vdp2blit_filter_f;
                            shader << Yglprg_vdp2_common_part;
                            shader << Yglprg_vdp2_common_part_screen[m];
                            shader << Yglprg_vdp2_common_end[m];
                            shader << vdp2blit_gl_end_f;
                            shader << Yglprg_vdp2_common_final[m];
                            shader << vdp2blit_gl_final_f;

                            auto const shaderCode = shader.str();
                            shaders.emplace_back(index, shaderCode);
                            unique_shaders.emplace(shaderCode);
                        }
                    }
                }
            }
        }
    }

    std::cout << "Shaders: " << shaders.size() << std::endl << "Unique Shaders:" << unique_shaders.size() << std::endl;

    auto const * outPath = "";
    if (argc > 1 && std::filesystem::exists(argv[1]))
    {
        outPath = argv[1];
    }

    std::for_each(std::execution::par_unseq, shaders.begin(), shaders.end(), [&](auto && tuple)
    {
        auto const & [index, shader] = tuple;
        std::ostringstream fileName;
        fileName << outPath;
        fileName << "kronos_shader_" << index << "_.glsl";

        std::ofstream file;
        file.open(fileName.str(), std::ios::out);
        file << shader;
        file.close();
    });

    //std::ofstream file;
    //for (auto const & [index, shader] : shaders)
    //{
    //    std::ostringstream fileName;
    //    fileName << "kronos_shader_" << index << "_.glsl";

    //    file.open(fileName.str(), std::ios::out);
    //    file << shader;
    //    file.close();
    //}
}