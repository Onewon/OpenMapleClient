// Regression tests for the Aran job tree and inherited combat skill levels.
#include "../app/src/main/cpp/src/Character/AranSkills.h"
#include "../app/src/main/cpp/src/Character/Job.h"
#include "../app/src/main/cpp/src/Character/SkillBook.h"
#include <cassert>

int main() {
    using namespace ms;
    const int jobs[]={2100,2110,2111,2112};
    const int skills[]={21000002,21100001,21110002,21120002};
    for(int tier=0;tier<4;++tier) {
        Job job(jobs[tier]);
        assert(job.can_use(20001000));
        assert(!job.can_use(1001004));
        for(int skillTier=0;skillTier<4;++skillTier)
            assert(job.can_use(skills[skillTier])==(skillTier<=tier));
    }
    SkillBook book;
    // A learned upgrade arrives without hidden skills, as in UPDATE_SKILLS.
    book.set_skill(21110002,7,20,-1);
    assert(book.get_level(21110007)==7 && book.get_level(21110008)==7);
    // Ignore stale hidden levels after an SP reset or a mastery book change.
    book.set_skill(21110007,20,20,-1);
    book.set_skill(21110002,0,20,-1);
    assert(book.get_level(21110007)==0);
    book.set_skill(21120002,19,20,-1);
    assert(book.get_level(21120009)==19 && book.get_level(21120010)==19);
    assert(aran::swing(2,0,0)==21000002);
    assert(aran::swing(3,0,0)==21100001);
    assert(aran::swing(2,20,0)==21110007);
    assert(aran::swing(3,20,1)==21120010);
}
