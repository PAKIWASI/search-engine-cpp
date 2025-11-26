import sys
import spacy
import re
from collections import Counter

            #LEMMATIZER WITH BETTER PROCESSING OF EDGE CASES FUCK YEAH

try:
    nlp = spacy.load("en_core_web_sm")
except OSError:
    print("no spacy", file=sys.stderr)
    sys.exit(1)


MEDICAL_PRESERVE = {
    'covid', 'covid-19', 'covid19', 'sars', 'sars-cov', 'sars-cov-2', 
    'mers', 'mers-cov', 'coronavirus', 'coronaviruses',
    'antibody', 'antibodies', 'antigen', 'antigens', 'vaccine', 'vaccines', 'vaccination',
    'virus', 'viruses', 'viral', 'virion', 'virions', 'protein', 'proteins', 'peptide', 'peptides',
    'rna', 'dna', 'mrna', 'trna', 'rrna', 'genome', 'genomic', 'gene', 'genes',
    'cell', 'cells', 'cellular', 'cytokine', 'cytokines', 'pneumonia', 'influenza', 'respiratory',
    'pulmonary', 'syndrome', 'disease', 'infection', 'infectious', 'transmission', 'transmissible',
    'contagious', 'symptom', 'symptoms', 'asymptomatic', 'symptomatic', 'diagnosis', 'diagnostic',
    'prognosis', 'prognostic', 'treatment', 'therapeutic', 'therapy', 'therapies', 'immune', 'immunity',
    'immunology', 'immunological', 'pathogen', 'pathogens', 'pathogenic', 'pathogenesis', 'pandemic',
    'epidemic', 'endemic', 'outbreak', 'mortality', 'morbidity', 'fatality', 'patient', 'patients',
    'clinical', 'hospital', 'icu', 'intensive', 'ventilator', 'ventilation', 'oxygen', 'hypoxia',
    'hypoxic', 'inflammation', 'inflammatory', 'receptor', 'receptors', 'ace2', 'spike', 'nucleocapsid',
    'membrane', 'antibacterial', 'antiviral', 'antimicrobial'
}

MEDICAL_ABBREV = {
    'who', 'cdc', 'fda', 'nih', 'niaid', 'ema', 'pcr', 'rt-pcr', 'qrt-pcr', 'elisa',
    'ards', 'icu', 'ecmo', 'il', 'il-6', 'il-1', 'tnf', 'tnf-alpha', 'ifn', 'igg', 'igm', 
    'iga', 'ige', 'hiv', 'aids', 'tb', 'ebv', 'cmv', 'ct', 'mri', 'xray', 'ecg', 'ekg',
    'mg', 'ml', 'kg', 'mcg', 'ng', 'pg', 'µg', 'usa', 'uk', 'eu'
}

NOISE_WORDS = {
    'vs', 'ad', 'ed', 'll', 'b', 'n', 'a', 'r', 'see', 'october', 'bs', 'epal', 'z'
}

def is_likely_noise(text):
    """Check if text is likely noise"""
    if len(text) == 1 and text not in {'a', 'i'} and text not in MEDICAL_ABBREV:
        return True
    if text in NOISE_WORDS:
        return True
    if re.search(r'^[-.,|/\\]|[-.,|/\\]$', text):
        return True
    if re.search(r'https?://|www\.|\.(com|org|edu|gov|net|html?|php|asp|pdf)', text):
        return True
    if re.search(r'^[acgtu]{10,}$', text.replace("'", "")):
        return True
    if len(text) > 25:
        return True
    if sum(c.isdigit() for c in text) > len(text) * 0.3:
        return True
    return False

def lemmatize_text(text):
    doc = nlp(text)
    lexicon = Counter()
    
    for token in doc:
        if token.is_punct or token.is_space or token.like_num:
            continue
            
        token_lower = token.text.lower()
        
        # Skip short tokens and noise
        if len(token_lower) < 2 and token_lower not in MEDICAL_ABBREV:
            continue
        if is_likely_noise(token_lower):
            continue
        if token.is_stop and token_lower not in MEDICAL_PRESERVE:
            continue
            
        # Preserve medical terms
        if token_lower in MEDICAL_PRESERVE or token_lower in MEDICAL_ABBREV:
            lexicon[token_lower] += 1
            continue
            
        # Get lemma
        lemma = token.lemma_.lower().strip()
        if not lemma or len(lemma) < 2:
            continue
        if is_likely_noise(lemma):
            continue
            
        lexicon[lemma] += 1
    
    return lexicon

def main():
    text = sys.stdin.read()
    
    if not text.strip():
        print("ERROR: No input text received", file=sys.stderr)
        sys.exit(1)
    
    lexicon = lemmatize_text(text)
    
    # Output CSV
    print("word,frequency")
    for word, freq in lexicon.most_common():
        word_escaped = word.replace(',', '_')
        print(f"{word_escaped},{freq}")

if __name__ == "__main__":
    main()


