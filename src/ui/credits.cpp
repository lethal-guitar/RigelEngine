/* Copyright (C) 2022, Nikolai Wuttke. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "credits.hpp"

#include "base/warnings.hpp"

RIGEL_DISABLE_WARNINGS
#include <imgui.h>
RIGEL_RESTORE_WARNINGS


namespace rigel::ui
{

void drawCredits(const float creditsBoxWidth)
{
  auto centeredText = [&](const char* text) {
    const auto textSize = ImGui::CalcTextSize(text).x;
    ImGui::SetCursorPosX((creditsBoxWidth - textSize) / 2.0f);
    ImGui::TextUnformatted(text);
  };

  auto centeredTextBig = [&](const char* text, const float factor = 1.8f) {
    auto pFont = ImGui::GetFont();
    const auto currentScale = pFont->Scale;

    pFont->Scale *= factor;
    ImGui::PushFont(pFont);
    centeredText(text);

    pFont->Scale = currentScale;
    ImGui::PopFont();
  };


  centeredTextBig("RigelEngine", 2.2f);
  ImGui::Spacing();
  centeredText("A modern re-implementation of the game Duke Nukem II");
  centeredText("(originally released in 1993 by Apogee Software)");

  ImGui::NewLine();

  centeredText("Created by Nikolai Wuttke (https://github.com/lethal_guitar)");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("With major contributions from");
  ImGui::Spacing();
  centeredText("Alain Martin (https://github.com/McMartin)");
  centeredText("Andrei-Florin Bencsik (https://github.com/bencsikandrei)");
  centeredText("Pratik Anand (https://github.com/pratikone)");
  centeredText("Ryan Brown (https://github.com/rbrown46)");
  centeredText("Soham Roy (https://github.com/sohamroy19)");
  centeredText("PatriotRossii (https://github.com/PatriotRossii)");
  centeredText("s-martin (https://github.com/s-martin)");

  ImGui::NewLine();

  centeredText("RigelEngine icon by LunarLoony (https://lunarloony.co.uk)");

  ImGui::NewLine();

  centeredText(
    "'Remixed 1' HUD artwork by Roobar (https://www.youtube.com/user/JBWhiskey)");
  centeredText(
    "'Remixed 2' HUD artwork by OpenRift412 (https://github.com/OpenRift412)");

  ImGui::NewLine();

  centeredText("openSUSE package by mnhauke (https://github.com/mnhauke)");

  ImGui::NewLine();

  centeredText("Many thanks to everyone else who contributed to the project!");
  centeredText("See AUTHORS.md on GitHub for a full list of contributors.");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("Special thanks");
  ImGui::Spacing();
  centeredText("Apogee Software");
  centeredText(
    "Shikadi Modding Wiki (https://moddingwiki.shikadi.net/wiki/Main_Page)");
  centeredText("The DOSBox project");
  centeredText("IDA Pro disassembler by Hex Rays");
  centeredText("Anton Gulenko");
  centeredText(
    "Clint Basinger aka LGR (https://www.youtube.com/c/Lazygamereviews)");
  centeredText("Bart aka Dosgamert (https://www.youtube.com/c/dosgamert)");
  centeredText("MaxiTaxi Creative (https://www.instagram.com/maxitaxi.f500)");
  centeredText("Everyone on the RigelEngine Discord");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredText("Rigel Engine Copyright (C) 2016, Nikolai Wuttke.");
  centeredText("Rigel Engine comes with ABSOLUTELY NO WARRANTY.");
  centeredText(
    "This is free software, and you are welcome to redistribute it under certain conditions.");
  centeredText("For details, see https://www.gnu.org/licenses/gpl-2.0.html.");

  ImGui::NewLine();

  centeredText("Find the full source code on GitHub:");
  centeredText("https://github.com/lethal-guitar/RigelEngine");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("Open-source components & licenses");

  ImGui::NewLine();

  centeredText("RigelEngine incorporates some 3rd party open source code.");
  centeredText("The following libraries and components are used:");

  ImGui::NewLine();

  centeredTextBig("Simple DirectMedia Layer (SDL)", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (C) 1997-2021 Sam Lantinga <slouken@libsdl.org>");
  centeredText("https://www.libsdl.org");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("SDL Mixer", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (C) 1997-2021 Sam Lantinga <slouken@libsdl.org>");
  centeredText("https://www.libsdl.org/projects/SDL_mixer");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("Lyra", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright 2018-2019 René Ferdinand Rivera Morell");
  centeredText("Copyright 2017 Two Blue Cubes Ltd. All rights reserved.");
  centeredText("https://github.com/bfgroup/Lyra");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("DBOPL AdLib emulator (from DosBox)", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (C) 2002-2021  The DOSBox Team");
  ImGui::NewLine();
  centeredText(
    "This program is free software; you can redistribute it and/or modify");
  centeredText(
    "it under the terms of the GNU General Public License as published by");
  centeredText(
    "the Free Software Foundation; either version 2 of the License, or");
  centeredText("(at your option) any later version.");
  ImGui::NewLine();
  centeredText(
    "This program is distributed in the hope that it will be useful,");
  centeredText(
    "but WITHOUT ANY WARRANTY; without even the implied warranty of");
  centeredText("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the");
  centeredText("GNU General Public License for more details.");
  ImGui::NewLine();
  centeredText("https://www.gnu.org/licenses/gpl-2.0.html");
  ImGui::NewLine();
  centeredText("https://sourceforge.net/projects/dosbox");
  centeredText("https://github.com/dosbox-staging/dosbox-staging");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("Nuked OPL3 AdLib emulator", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (C) 2013-2020 Nuke.YKT");
  ImGui::NewLine();
  centeredText(
    "Nuked OPL3 is free software: you can redistribute it and/or modify");
  centeredText(
    "it under the terms of the GNU Lesser General Public License as");
  centeredText("published by the Free Software Foundation, either version 2.1");
  centeredText("of the License, or (at your option) any later version.");
  ImGui::NewLine();
  centeredText("Nuked OPL3 is distributed in the hope that it will be useful,");
  centeredText(
    "but WITHOUT ANY WARRANTY; without even the implied warranty of");
  centeredText("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the");
  centeredText("GNU Lesser General Public License for more details.");
  ImGui::NewLine();
  centeredText("https://www.gnu.org/licenses/lgpl-2.1.html");
  ImGui::NewLine();
  centeredText("https://github.com/nukeykt/Nuked-OPL3");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("EntityX", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (C) 2012 Alec Thomas");
  ImGui::NewLine();
  centeredText(
    "Permission is hereby granted, free of charge, to any person obtaining a copy of");
  centeredText(
    "this software and associated documentation files (the \"Software\"), to deal in");
  centeredText(
    "the Software without restriction, including without limitation the rights to");
  centeredText(
    "use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies");
  centeredText(
    "of the Software, and to permit persons to whom the Software is furnished to do");
  centeredText("so, subject to the following conditions:");
  ImGui::NewLine();
  centeredText(
    "The above copyright notice and this permission notice shall be included in all");
  centeredText("copies or substantial portions of the Software.");
  ImGui::NewLine();
  centeredText(
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR");
  centeredText(
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,");
  centeredText(
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE");
  centeredText(
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER");
  centeredText(
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,");
  centeredText(
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE");
  centeredText("SOFTWARE.");
  ImGui::NewLine();
  centeredText("https://github.com/alecthomas/entityx");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("OpenGL/OpenGL ES loader code generated via glad", 1.4f);
  ImGui::Spacing();
  centeredText(
    "The glad code generator is Copyright (c) 2013-2021 David Herberth.");
  ImGui::Spacing();
  centeredText(
    "The generated code is in the public domain, except for khrplatform.h.");
  centeredText("The latter is under the following license:");
  ImGui::NewLine();
  centeredText("Copyright (c) 2008-2018 The Khronos Group Inc.");
  ImGui::NewLine();
  centeredText(
    "Permission is hereby granted, free of charge, to any person obtaining a");
  centeredText(
    "copy of this software and/or associated documentation files (the");
  centeredText(
    "\"Materials\"), to deal in the Materials without restriction, including");
  centeredText(
    "without limitation the rights to use, copy, modify, merge, publish,");
  centeredText(
    "distribute, sublicense, and/or sell copies of the Materials, and to");
  centeredText(
    "permit persons to whom the Materials are furnished to do so, subject to");
  centeredText("the following conditions:");
  ImGui::NewLine();
  centeredText(
    "The above copyright notice and this permission notice shall be included");
  centeredText("in all copies or substantial portions of the Materials.");
  ImGui::NewLine();
  centeredText(
    "THE MATERIALS ARE PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,");
  centeredText(
    "EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF");
  centeredText(
    "MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.");
  centeredText(
    "IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY");
  centeredText(
    "CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,");
  centeredText(
    "TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE");
  centeredText("MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.");
  ImGui::NewLine();
  centeredText("https://github.com/Dav1dde/glad");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("OpenGL Mathematics (GLM)", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (c) 2005 - G-Truc Creation");
  ImGui::NewLine();
  centeredText(
    "Permission is hereby granted, free of charge, to any person obtaining a copy of");
  centeredText(
    "this software and associated documentation files (the \"Software\"), to deal in");
  centeredText(
    "the Software without restriction, including without limitation the rights to");
  centeredText(
    "use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies");
  centeredText(
    "of the Software, and to permit persons to whom the Software is furnished to do");
  centeredText("so, subject to the following conditions:");
  ImGui::NewLine();
  centeredText(
    "The above copyright notice and this permission notice shall be included in all");
  centeredText("copies or substantial portions of the Software.");
  ImGui::NewLine();
  centeredText("Restrictions:");
  centeredText(
    " By making use of the Software for military purposes, you choose to make a");
  centeredText(" Bunny unhappy.");
  ImGui::NewLine();
  centeredText(
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR");
  centeredText(
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,");
  centeredText(
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE");
  centeredText(
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER");
  centeredText(
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,");
  centeredText(
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE");
  centeredText("SOFTWARE.");
  ImGui::NewLine();
  centeredText("https://github.com/g-truc/glm");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("Dear ImGui", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (c) 2014-2021 Omar Cornut");
  ImGui::NewLine();
  centeredText(
    "Permission is hereby granted, free of charge, to any person obtaining a copy of");
  centeredText(
    "this software and associated documentation files (the \"Software\"), to deal in");
  centeredText(
    "the Software without restriction, including without limitation the rights to");
  centeredText(
    "use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies");
  centeredText(
    "of the Software, and to permit persons to whom the Software is furnished to do");
  centeredText("so, subject to the following conditions:");
  ImGui::NewLine();
  centeredText(
    "The above copyright notice and this permission notice shall be included in all");
  centeredText("copies or substantial portions of the Software.");
  ImGui::NewLine();
  centeredText(
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR");
  centeredText(
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,");
  centeredText(
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE");
  centeredText(
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER");
  centeredText(
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,");
  centeredText(
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE");
  centeredText("SOFTWARE.");
  ImGui::NewLine();
  centeredText("https://github.com/ocornut/imgui");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("Dear ImGui file browser extension", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (c) 2019-2020 Zhuang Guan");
  ImGui::NewLine();
  centeredText(
    "Permission is hereby granted, free of charge, to any person obtaining a copy of");
  centeredText(
    "this software and associated documentation files (the \"Software\"), to deal in");
  centeredText(
    "the Software without restriction, including without limitation the rights to");
  centeredText(
    "use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies");
  centeredText(
    "of the Software, and to permit persons to whom the Software is furnished to do");
  centeredText("so, subject to the following conditions:");
  ImGui::NewLine();
  centeredText(
    "The above copyright notice and this permission notice shall be included in all");
  centeredText("copies or substantial portions of the Software.");
  ImGui::NewLine();
  centeredText(
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR");
  centeredText(
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,");
  centeredText(
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE");
  centeredText(
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER");
  centeredText(
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,");
  centeredText(
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE");
  centeredText("SOFTWARE.");
  ImGui::NewLine();
  centeredText("https://github.com/AirGuanZ/imgui-filebrowser");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("JSON for Modern C++", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (c) 2013-2019 Niels Lohmann <http://nlohmann.me>.");
  ImGui::NewLine();
  centeredText(
    "Permission is hereby granted, free of charge, to any person obtaining a copy of");
  centeredText(
    "this software and associated documentation files (the \"Software\"), to deal in");
  centeredText(
    "the Software without restriction, including without limitation the rights to");
  centeredText(
    "use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies");
  centeredText(
    "of the Software, and to permit persons to whom the Software is furnished to do");
  centeredText("so, subject to the following conditions:");
  ImGui::NewLine();
  centeredText(
    "The above copyright notice and this permission notice shall be included in all");
  centeredText("copies or substantial portions of the Software.");
  ImGui::NewLine();
  centeredText(
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR");
  centeredText(
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,");
  centeredText(
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE");
  centeredText(
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER");
  centeredText(
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,");
  centeredText(
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE");
  centeredText("SOFTWARE.");
  ImGui::NewLine();
  centeredText("https://json.nlohmann.me");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("Resampler code from libspeex", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (C) 2007 Jean-Marc Valin");
  ImGui::NewLine();
  centeredText(
    "Redistribution and use in source and binary forms, with or without");
  centeredText(
    "modification, are permitted provided that the following conditions are");
  centeredText("met:");
  ImGui::NewLine();
  centeredText(
    "1. Redistributions of source code must retain the above copyright notice,");
  centeredText("this list of conditions and the following disclaimer.");
  ImGui::NewLine();
  centeredText(
    "2. Redistributions in binary form must reproduce the above copyright");
  centeredText(
    "notice, this list of conditions and the following disclaimer in the");
  centeredText(
    "documentation and/or other materials provided with the distribution.");
  ImGui::NewLine();
  centeredText(
    "3. The name of the author may not be used to endorse or promote products");
  centeredText(
    "derived from this software without specific prior written permission.");
  ImGui::NewLine();
  centeredText(
    "THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR");
  centeredText(
    "IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES");
  centeredText("OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE");
  centeredText(
    "DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,");
  centeredText(
    "INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES");
  centeredText(
    "(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR");
  centeredText(
    "SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)");
  centeredText(
    "HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,");
  centeredText(
    "STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN");
  centeredText(
    "ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE");
  centeredText("POSSIBILITY OF SUCH DAMAGE.");
  ImGui::NewLine();
  centeredText("http://www.speex.org");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("static_vector", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright Gonzalo Brito Gadeschi 2015-2017");
  centeredText("Copyright Eric Niebler 2013-2014");
  centeredText("Copyright Casey Carter 2016");
  ImGui::NewLine();
  centeredText("https://github.com/gnzlbg/static_vector");

  ImGui::NewLine();
  ImGui::NewLine();

  centeredTextBig("stb_image & stb_rect_pack", 1.4f);
  ImGui::Spacing();
  centeredText("Copyright (c) 2017 Sean Barrett");
  ImGui::NewLine();
  centeredText(
    "Permission is hereby granted, free of charge, to any person obtaining a copy of");
  centeredText(
    "this software and associated documentation files (the \"Software\"), to deal in");
  centeredText(
    "the Software without restriction, including without limitation the rights to");
  centeredText(
    "use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies");
  centeredText(
    "of the Software, and to permit persons to whom the Software is furnished to do");
  centeredText("so, subject to the following conditions:");
  centeredText(
    "The above copyright notice and this permission notice shall be included in all");
  centeredText("copies or substantial portions of the Software.");
  ImGui::NewLine();
  centeredText(
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR");
  centeredText(
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,");
  centeredText(
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE");
  centeredText(
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER");
  centeredText(
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,");
  centeredText(
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE");
  centeredText("SOFTWARE.");
  ImGui::NewLine();
  centeredText("https://github.com/nothings/stb");
}

} // namespace rigel::ui
