#ifndef credits_hpp_include_guard
#define credits_hpp_include_guard

#include <El/elementpch.hpp>

struct CreditsInfo
{
  const char *name;
  const char *copyright;
  const char *disclaimer;

  CreditsInfo(){}
  CreditsInfo(const char *name, const char *copyright, const char *disclaimer):name(name),copyright(copyright),disclaimer(disclaimer){}
};

TypeIsSimple(CreditsInfo);

void RegisterCredits(const char *name, const char *copyright, const char *disclaimer);
int CountCredits();
const CreditsInfo &GetCredits(int i);

struct RegisterCreditsInit
{
  RegisterCreditsInit(const char *name, const char *copyright, const char *disclaimer)
  {
    RegisterCredits(name,copyright,disclaimer);
  }
};

#define UNIQUE_INIT2(x, y) x ## y
#define UNIQUE_INIT(x, y) UNIQUE_INIT2(x, y)

#define REGISTER_CREDITS(name,copyright,disclaimer) static RegisterCreditsInit UNIQUE_INIT(init,__LINE__)(name,copyright,disclaimer);

#define STANDARD_DISCLAIMER \
"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS' AND "\
"ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED "\
"WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE "\
"DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR "\
"ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES "\
"(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; "\
"LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON "\
"ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT "\
"(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS "\
"SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."

#endif
